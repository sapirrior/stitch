#include <stdlib.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/engine.h"
#include "stitch/ui/render.h"
#include "stitch/ui/prompt.h"
#include "../editor_internal.h"

void handle_normal_mode(StitchState *state, int c) {
    if (state->editor.last_key == 'd') {
        if (c == 'd') {
            if (state->view.cy < state->buffer.num_lines) {
                buffer_push_undo(&state->buffer, UNDO_DELETE_LINE, state->view.cy, 0, 0, state->buffer.lines[state->view.cy].chars, state->buffer.lines[state->view.cy].size);
                buffer_del_line(&state->buffer, state->view.cy);
                if (state->view.cy == state->buffer.num_lines && state->view.cy > 0) state->view.cy--;
                state->view.cx = 0;
            }
        }
        state->editor.last_key = 0;
        return;
    }

    switch (c) {
        case 'i':
            state->editor.last_key = 0;
            state->editor.mode = MODE_INSERT;
            ui_set_status_message(state, "-- INSERT --");
            break;
        case 'a':
            if (state->view.cy < state->buffer.num_lines) {
                Line *line = &state->buffer.lines[state->view.cy];
                if (state->view.cx < line->size) state->view.cx++;
            }
            state->editor.last_key = 0;
            state->editor.mode = MODE_INSERT;
            ui_set_status_message(state, "-- INSERT --");
            break;
        case 'A':
            if (state->view.cy < state->buffer.num_lines) state->view.cx = state->buffer.lines[state->view.cy].size;
            state->editor.last_key = 0;
            state->editor.mode = MODE_INSERT;
            ui_set_status_message(state, "-- INSERT --");
            break;
        case 'o': {
            size_t insert_at = (state->view.cy < state->buffer.num_lines) ? state->view.cy + 1 : state->buffer.num_lines;
            buffer_insert_line(&state->buffer, insert_at, "", 0);
            buffer_push_undo(&state->buffer, UNDO_INSERT_LINE, insert_at, 0, 0, NULL, 0);
            state->view.cy = insert_at;
            state->view.cx = 0;
            state->editor.mode = MODE_INSERT;
            ui_set_status_message(state, "-- INSERT --");
            break;
        }
        case 'O':
            buffer_insert_line(&state->buffer, state->view.cy, "", 0);
            buffer_push_undo(&state->buffer, UNDO_INSERT_LINE, state->view.cy, 0, 0, NULL, 0);
            state->view.cx = 0;
            state->editor.last_key = 0;
            state->editor.mode = MODE_INSERT;
            ui_set_status_message(state, "-- INSERT --");
            break;
        case 'v':
            state->editor.last_key = 0;
            state->editor.mode = MODE_VISUAL;
            state->editor.visual_cx = state->view.cx;
            state->editor.visual_cy = state->view.cy;
            ui_set_status_message(state, "-- VISUAL --");
            break;
        case 'u':
            buffer_undo(&state->buffer, &state->view);
            break;
        case 'U':
            buffer_redo(&state->buffer, &state->view);
            break;
        case ':':
            state->editor.last_key = 0;
            state->editor.mode = MODE_COMMAND;
            handle_command_prompt_mode(state, c);
            break;
        case '/':
            state->editor.last_key = 0;
            cmd_search_execute(state);
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
        case 'x':
            if (state->view.cy < state->buffer.num_lines) {
                Line *line = &state->buffer.lines[state->view.cy];
                if (state->view.cx < line->size) {
                    /* Move cx to the start of the next character */
                    state->view.cx++;
                    while (state->view.cx < line->size && !editorIsUtf8Start((unsigned char)line->chars[state->view.cx])) {
                        state->view.cx++;
                    }
                    buffer_del_char(&state->buffer, &state->view);
                }
            }
            break;
        case 'd':
            state->editor.last_key = 'd';
            break;
        case STITCH_PAGE_UP:
        case STITCH_PAGE_DOWN:
            {
                if (c == STITCH_PAGE_UP) state->view.cy = state->view.row_off;
                else if (c == STITCH_PAGE_DOWN) {
                    state->view.cy = state->view.row_off + (size_t)state->view.screen_rows - 1;
                    if (state->view.cy > state->buffer.num_lines) state->view.cy = state->buffer.num_lines;
                }
                int times = state->view.screen_rows;
                while (times--)
                    editor_move_cursor(state, c == STITCH_PAGE_UP ? STITCH_ARROW_UP : STITCH_ARROW_DOWN);
            }
            break;
    }
}
