#include <string.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "../editor_internal.h"

void editor_move_cursor(StitchState *state, int key) {
    Line *line = (state->view.cy >= state->buffer.num_lines) ? NULL : &state->buffer.lines[state->view.cy];

    switch (key) {
        case 'h':
        case STITCH_ARROW_LEFT:
            if (!line) { state->view.cx = 0; break; }
            if (state->view.cx > 0) {
                state->view.cx--;
                while (state->view.cx > 0 && !editorIsUtf8Start((unsigned char)line->chars[state->view.cx])) 
                    state->view.cx--;
            } else if (state->view.cy > 0) {
                state->view.cy--;
                state->view.cx = state->buffer.lines[state->view.cy].size;
            }
            break;
        case 'l':
        case STITCH_ARROW_RIGHT:
            if (line && state->view.cx < line->size) {
                state->view.cx++;
                while (state->view.cx < line->size && !editorIsUtf8Start((unsigned char)line->chars[state->view.cx])) 
                    state->view.cx++;
            } else if (line && state->view.cx == line->size) {
                if (state->view.cy + 1 < state->buffer.num_lines) {
                    state->view.cy++;
                    state->view.cx = 0;
                }
            }
            break;
        case 'k':
        case STITCH_ARROW_UP:
            if (state->view.cy > 0) state->view.cy--;
            break;
        case 'j':
        case STITCH_ARROW_DOWN:
            if (state->view.cy + 1 < state->buffer.num_lines) state->view.cy++;
            break;
    }

    line = (state->view.cy >= state->buffer.num_lines) ? NULL : &state->buffer.lines[state->view.cy];
    size_t linelen = line ? line->size : 0;
    if (state->view.cx > linelen) state->view.cx = linelen;
}
