#include <ncurses.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "ui_internal.h"

int ui_get_gutter_width(StitchState *state) {
    if (!state->ui.show_line_numbers) return 0;
    int digits = 1;
    size_t n = state->buffer.num_lines;
    if (n == 0) n = 1;
    while (n >= 10) {
        digits++;
        n /= 10;
    }
    return digits + 2; /* e.g. " 1 " */
}

void ui_update_viewport(StitchState *state) {
    int gutter_width = ui_get_gutter_width(state);
    int available_cols = state->view.screen_cols - gutter_width;
    if (available_cols < 1) available_cols = 1;
    if (state->view.screen_rows < 1) state->view.screen_rows = 1;

    state->view.rx = 0;
    if (state->view.cy < state->buffer.num_lines) {
        Line *line = &state->buffer.lines[state->view.cy];
        for (size_t j = 0; j < state->view.cx && j < line->size; j++) {
            unsigned char c = (unsigned char)line->chars[j];
            if (c == '\t') {
                state->view.rx += (STITCH_TAB_STOP - (state->view.rx % STITCH_TAB_STOP));
            } else if (c < 32 || c == 127) {
                state->view.rx += 2;
            } else if (editorIsUtf8Start(c)) {
                state->view.rx++;
            }
        }
    }

    /* Vertical Scroll Margins (scrolloff) */
    int margin_y = 3;
    if (margin_y > state->view.screen_rows / 2) margin_y = state->view.screen_rows / 2;

    if ((size_t)state->view.screen_rows > (size_t)margin_y) {
        if (state->view.cy < state->view.row_off + (size_t)margin_y) {
            state->view.row_off = (state->view.cy < (size_t)margin_y) ? 0 : state->view.cy - (size_t)margin_y;
        }
        if (state->view.cy >= state->view.row_off + (size_t)state->view.screen_rows - (size_t)margin_y) {
            state->view.row_off = (state->view.cy + (size_t)margin_y + 1 > (size_t)state->view.screen_rows) ? 
                                  state->view.cy - (size_t)state->view.screen_rows + (size_t)margin_y + 1 : 0;
        }
    }

    /* Horizontal Scroll Margins (sidescrolloff) */
    int margin_x = 5;
    if (margin_x > available_cols / 2) margin_x = available_cols / 2;

    if ((size_t)available_cols > (size_t)margin_x) {
        if (state->view.rx < state->view.col_off + (size_t)margin_x) {
            state->view.col_off = (state->view.rx < (size_t)margin_x) ? 0 : state->view.rx - (size_t)margin_x;
        }
        if (state->view.rx >= state->view.col_off + (size_t)available_cols - (size_t)margin_x) {
            state->view.col_off = (state->view.rx + (size_t)margin_x + 1 > (size_t)available_cols) ?
                                  state->view.rx - (size_t)available_cols + (size_t)margin_x + 1 : 0;
        }
    }
}

void ui_refresh_screen(StitchState *state) {
    ui_update_viewport(state);

    ui_text_grid_draw(state);
    ui_status_bar_draw(state);
    ui_message_bar_draw(state);
    
    ui_help_overlay_draw(state);

    /* Position cursor */
    if (state->editor.mode == MODE_COMMAND) {
        size_t msg_len = strlen(state->ui.status_msg);
        size_t msg_cols = editorRowByteToCol(state->ui.status_msg, msg_len, msg_len);
        move(state->view.screen_rows + 1, (int)msg_cols);
    } else {
        int gutter_width = ui_get_gutter_width(state);
        int cursor_y = (int)(state->view.cy - state->view.row_off);
        int cursor_x = (int)(state->view.rx - state->view.col_off) + gutter_width;
        
        /* Ensure cursor stays within screen bounds */
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_y > state->view.screen_rows) cursor_y = state->view.screen_rows;
        if (cursor_x < gutter_width) cursor_x = gutter_width;
        if (cursor_x >= state->view.screen_cols) cursor_x = state->view.screen_cols - 1;
        
        move(cursor_y, cursor_x);
    }

    wnoutrefresh(stdscr);
    doupdate();
}

void ui_set_status_message(StitchState *state, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(state->ui.status_msg, sizeof(state->ui.status_msg), fmt, ap);
    va_end(ap);
}

void ui_handle_resize(StitchState *state) {
    resizeterm(0, 0);
    clear();
    getmaxyx(stdscr, state->view.screen_rows, state->view.screen_cols);
    state->view.screen_rows -= 2;
    if (state->view.screen_rows < 0) state->view.screen_rows = 0;
    touchwin(stdscr);
}

void ui_screen_to_buffer(StitchState *state, int screen_y, int screen_x, size_t *out_cy, size_t *out_cx) {
    int gutter_width = ui_get_gutter_width(state);
    
    /* Calculate Buffer Line */
    size_t target_cy = screen_y + state->view.row_off;
    if (target_cy >= state->buffer.num_lines) {
        *out_cy = state->buffer.num_lines > 0 ? state->buffer.num_lines - 1 : 0;
    } else {
        *out_cy = target_cy;
    }

    /* Calculate Buffer Column */
    int target_rx = screen_x - gutter_width + (int)state->view.col_off;
    if (target_rx < 0) target_rx = 0;

    if (*out_cy < state->buffer.num_lines) {
        Line *line = &state->buffer.lines[*out_cy];
        *out_cx = editorRowColToByte(line->chars, (int)line->size, target_rx);
    } else {
        *out_cx = 0;
    }
}
