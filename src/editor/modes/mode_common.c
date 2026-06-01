#include <string.h>
#include <ctype.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "../editor_internal.h"

static bool is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

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

void editor_move_word_forward(StitchState *state) {
    if (state->view.cy >= state->buffer.num_lines) return;
    Line *line = &state->buffer.lines[state->view.cy];
    if (state->view.cx >= line->size) {
        if (state->view.cy + 1 < state->buffer.num_lines) {
            state->view.cy++;
            state->view.cx = 0;
            line = &state->buffer.lines[state->view.cy];
            while (state->view.cx < line->size && isspace((unsigned char)line->chars[state->view.cx])) {
                state->view.cx++;
            }
        }
        return;
    }

    bool start_is_word = is_word_char(line->chars[state->view.cx]);
    
    while (state->view.cx < line->size) {
        char c = line->chars[state->view.cx];
        if (isspace((unsigned char)c)) break;
        if (is_word_char(c) != start_is_word) break;
        state->view.cx++;
    }
    
    while (state->view.cx < line->size && isspace((unsigned char)line->chars[state->view.cx])) {
        state->view.cx++;
    }
    
    if (state->view.cx >= line->size && state->view.cy + 1 < state->buffer.num_lines) {
        state->view.cy++;
        state->view.cx = 0;
        line = &state->buffer.lines[state->view.cy];
        while (state->view.cx < line->size && isspace((unsigned char)line->chars[state->view.cx])) {
            state->view.cx++;
        }
    }
}

void editor_move_word_backward(StitchState *state) {
    if (state->view.cy >= state->buffer.num_lines) return;
    Line *line = &state->buffer.lines[state->view.cy];
    
    if (state->view.cx == 0) {
        if (state->view.cy > 0) {
            state->view.cy--;
            line = &state->buffer.lines[state->view.cy];
            state->view.cx = line->size;
        } else {
            return;
        }
    }
    
    state->view.cx--;
    
    while (state->view.cx > 0 && isspace((unsigned char)line->chars[state->view.cx])) {
        state->view.cx--;
    }
    if (state->view.cx == 0 && isspace((unsigned char)line->chars[state->view.cx])) {
        return;
    }
    
    bool is_word = is_word_char(line->chars[state->view.cx]);
    while (state->view.cx > 0) {
        char c = line->chars[state->view.cx - 1];
        if (isspace((unsigned char)c)) break;
        if (is_word_char(c) != is_word) break;
        state->view.cx--;
    }
}

void editor_find_char(StitchState *state, int c) {
    if (state->view.cy >= state->buffer.num_lines) return;
    Line *line = &state->buffer.lines[state->view.cy];
    for (size_t i = state->view.cx + 1; i < line->size; i++) {
        if (line->chars[i] == c) {
            state->view.cx = i;
            break;
        }
    }
}
