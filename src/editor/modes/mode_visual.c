#include <stdlib.h>
#include <string.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/engine.h"
#include "stitch/ui/render.h"
#include "../editor_internal.h"

static void get_visual_bounds(StitchState *state, size_t *sy, size_t *sx, size_t *ey, size_t *ex) {
    if (state->view.cy < state->editor.visual_cy || (state->view.cy == state->editor.visual_cy && state->view.cx < state->editor.visual_cx)) {
        *sy = state->view.cy;
        *sx = state->view.cx;
        *ey = state->editor.visual_cy;
        *ex = state->editor.visual_cx;
    } else {
        *sy = state->editor.visual_cy;
        *sx = state->editor.visual_cx;
        *ey = state->view.cy;
        *ex = state->view.cx;
    }
}

static void delete_visual_block(StitchState *state, size_t sy, size_t sx, size_t ey, size_t ex) {
    state->buffer.group_undo = true;
    state->buffer.disable_update_line = true;
    
    state->view.cy = ey;
    state->view.cx = ex;

    if (state->view.cy < state->buffer.num_lines) {
        Line *line = &state->buffer.lines[state->view.cy];
        if (state->view.cx < line->size) {
            state->view.cx++;
            while (state->view.cx < line->size && !editorIsUtf8Start((unsigned char)line->chars[state->view.cx])) {
                state->view.cx++;
            }
        } else {
            if (state->view.cy < state->buffer.num_lines - 1) {
                state->view.cy++;
                state->view.cx = 0;
            }
        }
    }

    while (state->view.cy > sy || (state->view.cy == sy && state->view.cx > sx)) {
        size_t old_cy = state->view.cy;
        size_t old_cx = state->view.cx;
        buffer_del_char(&state->buffer, &state->view);
        if (state->view.cy == old_cy && state->view.cx == old_cx) break;
    }
    
    if (state->view.cy >= state->buffer.num_lines) {
        state->view.cy = state->buffer.num_lines > 0 ? state->buffer.num_lines - 1 : 0;
    }
    if (state->buffer.num_lines > 0) {
        if (state->view.cx > state->buffer.lines[state->view.cy].size) {
            state->view.cx = state->buffer.lines[state->view.cy].size;
        }
    } else {
        state->view.cx = 0;
    }

    state->buffer.disable_update_line = false;
    state->buffer.group_undo = false;
    
    if (state->view.cy < state->buffer.num_lines) {
        buffer_update_line(&state->buffer.lines[state->view.cy]);
    }
}

void handle_visual_mode(StitchState *state, int c) {
    size_t sy, sx, ey, ex;

    switch (c) {
        case '\x1b':
        case 'v':
            state->editor.mode = MODE_NORMAL;
            ui_set_status_message(state, "");
            break;
        case 'h':
        case 'j':
        case 'k':
        case 'l':
        case STITCH_ARROW_UP:
        case STITCH_ARROW_DOWN:
        case STITCH_ARROW_LEFT:
        case STITCH_ARROW_RIGHT:
            editor_move_cursor(state, c);
            break;
        case '0':
        case STITCH_HOME_KEY:
            state->view.cx = 0;
            break;
        case '$':
        case STITCH_END_KEY:
            if (state->view.cy < state->buffer.num_lines) state->view.cx = state->buffer.lines[state->view.cy].size;
            break;
        case 'g':
            state->view.cy = 0;
            state->view.cx = 0;
            break;
        case 'G':
            if (state->buffer.num_lines > 0) {
                state->view.cy = state->buffer.num_lines - 1;
            } else {
                state->view.cy = 0;
            }
            state->view.cx = 0;
            break;
        case '%':
            state->editor.visual_cy = 0;
            state->editor.visual_cx = 0;
            if (state->buffer.num_lines > 0) {
                state->view.cy = state->buffer.num_lines - 1;
                state->view.cx = state->buffer.lines[state->view.cy].size;
            } else {
                state->view.cy = 0;
                state->view.cx = 0;
            }
            break;
        case 'y':
            state->editor.mode = MODE_NORMAL;
            ui_set_status_message(state, "Yank temporarily disabled");
            break;
        case 'd':
        case 'x':
            get_visual_bounds(state, &sy, &sx, &ey, &ex);
            delete_visual_block(state, sy, sx, ey, ex);
            state->editor.mode = MODE_NORMAL;
            ui_set_status_message(state, "");
            break;
    }
}
