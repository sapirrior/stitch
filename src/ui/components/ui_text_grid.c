#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "ui_internal.h"

static int get_rx_from_cx(Line *line, size_t target_cx) {
    int rx = 0;
    for (size_t j = 0; j < target_cx && j < line->size; j++) {
        unsigned char c = (unsigned char)line->chars[j];
        if (c == '\t') {
            rx += (STITCH_TAB_STOP - (rx % STITCH_TAB_STOP));
        } else if (c < 32 || c == 127) {
            rx += 2;
        } else if (editorIsUtf8Start(c)) {
            rx++;
        }
    }
    return rx;
}

static bool find_matching_bracket(StitchState *state, size_t *match_y, size_t *match_x) {
    if (state->view.cy >= state->buffer.num_lines) return false;
    Line *line = &state->buffer.lines[state->view.cy];
    if (state->view.cx >= line->size) return false;

    char c = line->chars[state->view.cx];
    char pair;
    int dir = 0;
    
    if (c == '(') { pair = ')'; dir = 1; }
    else if (c == '[') { pair = ']'; dir = 1; }
    else if (c == '{') { pair = '}'; dir = 1; }
    else if (c == ')') { pair = '('; dir = -1; }
    else if (c == ']') { pair = '['; dir = -1; }
    else if (c == '}') { pair = '{'; dir = -1; }
    else return false;

    int depth = 1;
    size_t y = state->view.cy;
    size_t x = state->view.cx;

    size_t max_iter = state->buffer.num_lines * 2 + 1024;
    while (max_iter-- > 0) {
        if (dir == 1) {
            x++;
            if (x >= state->buffer.lines[y].size) {
                y++;
                if (y >= state->buffer.num_lines) return false;
                x = 0;
                continue;
            }
        } else {
            if (x == 0) {
                if (y == 0) return false;
                y--;
                x = state->buffer.lines[y].size;
                if (x > 0) x--;
                else continue;
            } else {
                x--;
            }
        }
        
        char cur = state->buffer.lines[y].chars[x];
        if (cur == c) depth++;
        else if (cur == pair) depth--;

        if (depth == 0) {
            *match_y = y;
            *match_x = x;
            return true;
        }
    }
    return false;
}

static bool get_visual_selection_bytes(StitchState *state, size_t filerow, size_t *start_byte, size_t *end_byte) {
    if (state->editor.mode != MODE_VISUAL) return false;
    
    size_t start_y, end_y, start_x, end_x;
    if (state->view.cy < state->editor.visual_cy || (state->view.cy == state->editor.visual_cy && state->view.cx < state->editor.visual_cx)) {
        start_y = state->view.cy;
        start_x = state->view.cx;
        end_y = state->editor.visual_cy;
        end_x = state->editor.visual_cx;
    } else {
        start_y = state->editor.visual_cy;
        start_x = state->editor.visual_cx;
        end_y = state->view.cy;
        end_x = state->view.cx;
    }

    if (filerow < start_y || filerow > end_y) return false;

    Line *line = &state->buffer.lines[filerow];
    size_t c_start = (filerow == start_y) ? start_x : 0;
    size_t c_end = (filerow == end_y) ? end_x : line->size;

    int rx_start = get_rx_from_cx(line, c_start);
    
    char *rline = line->render;
    size_t rlen = line->rsize;
    
    *start_byte = editorRowColToByte(rline, rlen, rx_start);
    
    if (c_end == line->size) {
        *end_byte = rlen;
        if (*start_byte == *end_byte) {
             /* Highlight the newline/end of line visually if it's an empty line or at the end */
             *end_byte = *start_byte + 1;
        }
    } else {
        size_t next_c = c_end;
        if (next_c < line->size) {
            next_c++;
            while (next_c < line->size && !editorIsUtf8Start((unsigned char)line->chars[next_c])) {
                next_c++;
            }
        }
        int next_rx = get_rx_from_cx(line, next_c);
        *end_byte = editorRowColToByte(rline, rlen, next_rx);
        if (*start_byte == *end_byte) *end_byte = *start_byte + 1; // Fallback
    }
    
    return true;
}

void ui_text_grid_draw(StitchState *state) {
    int gutter_width = ui_get_gutter_width(state);
    int available_cols = state->view.screen_cols - gutter_width;
    if (available_cols < 0) available_cols = 0;
    
    size_t match_y = 0, match_x = 0;
    bool has_match = find_matching_bracket(state, &match_y, &match_x);

    for (int y = 0; y < state->view.screen_rows; y++) {
        size_t filerow = (size_t)y + state->view.row_off;
        move(y, 0);
        clrtoeol();

        if (state->ui.show_line_numbers) {
            if (filerow < state->buffer.num_lines) {
                attron(A_DIM);
                mvprintw(y, 0, " %*zu", gutter_width - 2, filerow + 1);
                attroff(A_DIM);
            }
        }

        if (filerow >= state->buffer.num_lines) {
            if (state->buffer.num_lines == 0 && y == state->view.screen_rows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "Stitch editor -- version %s", STITCH_VERSION);
                if (welcomelen > available_cols) welcomelen = available_cols;
                int x = gutter_width + (available_cols - welcomelen) / 2;
                if (x < gutter_width) x = gutter_width;
                mvaddnstr(y, x, welcome, welcomelen);
            }
        } else {
            size_t rlen = state->buffer.lines[filerow].rsize;
            char *rline = state->buffer.lines[filerow].render;

            size_t coff = editorRowColToByte(rline, rlen, state->view.col_off);
            size_t draw_len = editorRowColToByte(&rline[coff], rlen - coff, (size_t)available_cols);

            size_t hl1_byte = (size_t)-1;
            size_t hl2_byte = (size_t)-1;
            if (has_match) {
                if (filerow == state->view.cy) {
                    int rx = get_rx_from_cx(&state->buffer.lines[filerow], state->view.cx);
                    if (rx >= (int)state->view.col_off && rx < (int)state->view.col_off + available_cols) {
                        hl1_byte = editorRowColToByte(rline, rlen, (size_t)rx) - coff;
                    }
                }
                if (filerow == match_y) {
                    int rx = get_rx_from_cx(&state->buffer.lines[filerow], match_x);
                    if (rx >= (int)state->view.col_off && rx < (int)state->view.col_off + available_cols) {
                        hl2_byte = editorRowColToByte(rline, rlen, (size_t)rx) - coff;
                    }
                }
            }
            
            size_t vis_start_byte = 0, vis_end_byte = 0;
            bool is_visual = get_visual_selection_bytes(state, filerow, &vis_start_byte, &vis_end_byte);

            move(y, gutter_width);
            for (size_t i = 0; i < draw_len; i++) {
                size_t file_byte_idx = coff + i;
                bool is_vis_highlight = is_visual && file_byte_idx >= vis_start_byte && file_byte_idx < vis_end_byte;
                
                if (is_vis_highlight) {
                    attron(A_REVERSE);
                }
                
                if (!is_vis_highlight && (i == hl1_byte || i == hl2_byte)) {
                    attron(COLOR_PAIR(2)); /* Terracotta for brackets */
                    addch(rline[file_byte_idx]);
                    attroff(COLOR_PAIR(2));
                } else {
                    addch(rline[file_byte_idx]);
                }
                
                if (is_vis_highlight) {
                    attroff(A_REVERSE);
                }
            }
            
            /* If visual selection extends past end of line, draw a reverse space to show it */
            if (is_visual && vis_end_byte > rlen && (coff + draw_len >= rlen)) {
                attron(A_REVERSE);
                addch(' ');
                attroff(A_REVERSE);
            }
        }
    }
}
