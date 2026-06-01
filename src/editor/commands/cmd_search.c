#include <string.h>
#include <stdlib.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/ui/render.h"
#include "stitch/ui/prompt.h"
#include "../editor_internal.h"

static void editor_find_callback(StitchState *state, char *query, int key) {
    static size_t last_match_cy = 0;
    static size_t last_match_cx = 0;
    static int direction = 1;
    static bool is_first_search = true;

    if (key == '\r' || key == '\x1b') {
        is_first_search = true;
        direction = 1;
        return;
    } else if (key == STITCH_ARROW_RIGHT || key == STITCH_ARROW_DOWN) {
        direction = 1;
    } else if (key == STITCH_ARROW_LEFT || key == STITCH_ARROW_UP) {
        direction = -1;
    } else {
        last_match_cy = state->view.cy;
        last_match_cx = state->view.cx;
        direction = 1;
        is_first_search = true;
    }

    if (query[0] == '\0') {
        free(state->editor.search_query);
        state->editor.search_query = NULL;
        return;
    }
    
    char *new_query = editorStrdup(query);
    free(state->editor.search_query);
    state->editor.search_query = new_query;

    if (state->buffer.num_lines == 0) return;

    size_t qlen = strlen(query);
    size_t current_cy = last_match_cy;
    size_t current_cx = last_match_cx;
    
    if (!is_first_search) {
        if (direction == 1) {
            current_cx++;
        } else {
            if (current_cx > 0) current_cx--;
            else {
                if (current_cy > 0) current_cy--;
                else current_cy = state->buffer.num_lines - 1;
                current_cx = state->buffer.lines[current_cy].size;
            }
        }
    }
    is_first_search = false;

    bool found = false;
    size_t num_lines = state->buffer.num_lines;
    
    for (size_t i = 0; i < num_lines; i++) {
        Line *line = &state->buffer.lines[current_cy];
        if (line->chars && line->size > 0) {
            if (direction == 1) {
                if (current_cx < line->size) {
                    char *match = editorStrcasestr(line->chars + current_cx, query);
                    if (match) {
                        current_cx = (size_t)(match - line->chars);
                        found = true;
                        break;
                    }
                }
            } else {
                /* Backward search within the line */
                ssize_t search_end = current_cx;
                if (search_end > (ssize_t)(line->size - qlen)) {
                    search_end = line->size - qlen;
                }
                for (ssize_t j = search_end; j >= 0; j--) {
                    if (editorStrcasestr(line->chars + j, query) == line->chars + j) {
                        current_cx = (size_t)j;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
        }
        
        if (direction == 1) {
            current_cy = (current_cy + 1) % num_lines;
            current_cx = 0;
        } else {
            current_cy = (current_cy == 0) ? num_lines - 1 : current_cy - 1;
            current_cx = state->buffer.lines[current_cy].size;
        }
    }

    if (found) {
        last_match_cy = current_cy;
        last_match_cx = current_cx;
        state->view.cy = current_cy;
        state->view.cx = current_cx;
        
        /* Center the match if it's outside the view */
        if (state->view.cy < state->view.row_off || state->view.cy >= state->view.row_off + state->view.screen_rows) {
            if (state->view.cy > (size_t)state->view.screen_rows / 2)
                state->view.row_off = state->view.cy - state->view.screen_rows / 2;
            else
                state->view.row_off = 0;
        }
        
        ui_set_status_message(state, "Match found (Arrows to navigate)");
    } else {
        ui_set_status_message(state, "No matches found");
    }
}

void cmd_search_execute(StitchState *state) {
    size_t saved_cx = state->view.cx;
    size_t saved_cy = state->view.cy;
    size_t saved_col_off = state->view.col_off;
    size_t saved_row_off = state->view.row_off;

    char *query = ui_prompt(state, "Search: %s (Use Arrows/Enter/Esc)",
                               editor_find_callback);

    if (query) {
        free(query);
    } else {
        state->view.cx = saved_cx;
        state->view.cy = saved_cy;
        state->view.col_off = saved_col_off;
        state->view.row_off = saved_row_off;
    }
}
