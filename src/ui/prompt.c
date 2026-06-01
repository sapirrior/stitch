#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <ncurses.h>
#include "stitch/core/terminal.h"
#include "stitch/ui/render.h"
#include "stitch/ui/prompt.h"

char *ui_prompt(StitchState *state, char *prompt, void (*callback)(StitchState *, char *, int)) {
    size_t bufsize = 128;
    char *buf = editorMalloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';

    int history_idx = state->editor.history_count;
    char *saved_buf = NULL;
    
    char *p = strstr(prompt, "%s");
    state->ui.prompt_cursor_offset = p ? (size_t)(p - prompt) : 0;
    state->ui.prompt_cursor = 0;

    while (1) {
        ui_set_status_message(state, prompt, buf);
        ui_refresh_screen(state);

        int c = core_read_key(state);

        if (c == STITCH_DEL_KEY || c == STITCH_BACKSPACE || c == CTRL_KEY('h')) {
            if (c == STITCH_DEL_KEY) {
                if (state->ui.prompt_cursor < buflen) {
                    memmove(&buf[state->ui.prompt_cursor], &buf[state->ui.prompt_cursor + 1], buflen - state->ui.prompt_cursor);
                    buflen--;
                }
            } else {
                if (state->ui.prompt_cursor > 0) {
                    memmove(&buf[state->ui.prompt_cursor - 1], &buf[state->ui.prompt_cursor], buflen - state->ui.prompt_cursor + 1);
                    state->ui.prompt_cursor--;
                    buflen--;
                }
            }
        } else if (c == STITCH_ARROW_LEFT) {
            if (state->ui.prompt_cursor > 0) state->ui.prompt_cursor--;
        } else if (c == STITCH_ARROW_RIGHT) {
            if (state->ui.prompt_cursor < buflen) state->ui.prompt_cursor++;
        } else if (c == '\x1b') {
            ui_set_status_message(state, "");
            if (callback) callback(state, buf, c);
            free(buf);
            free(saved_buf);
            return NULL;
        } else if (c == STITCH_ARROW_UP || c == STITCH_ARROW_DOWN) {
            if (state->editor.history_count == 0) continue;

            if (saved_buf == NULL) saved_buf = editorStrdup(buf);

            if (c == STITCH_ARROW_UP) {
                if (history_idx > 0) history_idx--;
            } else {
                if (history_idx < state->editor.history_count) history_idx++;
            }

            free(buf);
            if (history_idx < state->editor.history_count && state->editor.history[history_idx]) {
                bufsize = strlen(state->editor.history[history_idx]) + 1;
                buf = editorMalloc(bufsize);
                strcpy(buf, state->editor.history[history_idx]);
            } else {
                bufsize = strlen(saved_buf) + 1;
                buf = editorMalloc(bufsize);
                strcpy(buf, saved_buf);
            }
            buflen = strlen(buf);
            state->ui.prompt_cursor = buflen;
        } else if (c == '\r') {
            ui_set_status_message(state, "");
            if (callback) callback(state, buf, c);
            free(saved_buf);
            return buf;
        } else if (c >= 32 && c < 127) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = editorRealloc(buf, bufsize);
            }
            memmove(&buf[state->ui.prompt_cursor + 1], &buf[state->ui.prompt_cursor], buflen - state->ui.prompt_cursor + 1);
            buf[state->ui.prompt_cursor] = (char)c;
            state->ui.prompt_cursor++;
            buflen++;
        }

        if (callback) callback(state, buf, c);
    }
}
