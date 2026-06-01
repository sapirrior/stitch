#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/io.h"
#include "stitch/buffer/engine.h"
#include "stitch/ui/render.h"
#include "editor_internal.h"
#include "tutor_text.h"

void editor_add_history(StitchState *state, const char *cmd) {
    if (cmd == NULL || cmd[0] == '\0') return;

    if (state->editor.history_count > 0 && strcmp(state->editor.history[state->editor.history_count - 1], cmd) == 0) {
        return;
    }

    if (state->editor.history_count < 10) {
        state->editor.history[state->editor.history_count++] = editorStrdup(cmd);
    } else {
        free(state->editor.history[0]);
        memmove(&state->editor.history[0], &state->editor.history[1], sizeof(char *) * 9);
        state->editor.history[9] = editorStrdup(cmd);
    }
}

void editor_handle_command(StitchState *state, const char *cmd) {
    if (cmd[0] == '!') {
        if (cmd[1] == '\0') {
            ui_set_status_message(state, "Usage: !<command>");
            return;
        }
        cmd_shell_execute(state, cmd + 1);
        return;
    }

    if (strcmp(cmd, "q") == 0) {
        cmd_quit_execute(state);
    } else if (strcmp(cmd, "q!") == 0) {
        if (state->has_stash) {
            buffer_free(&state->buffer);
            int current_rows = state->view.screen_rows;
            int current_cols = state->view.screen_cols;
            state->buffer = state->stashed_buffer;
            state->view = state->stashed_view;
            state->view.screen_rows = current_rows;
            state->view.screen_cols = current_cols;
            state->has_stash = false;
            state->editor.mode = MODE_NORMAL;
            ui_handle_resize(state);
            ui_set_status_message(state, "Returned from tutor (discarded changes)");
            return;
        }
        exit(0);
    } else if (strcmp(cmd, "w") == 0) {
        cmd_save_execute(state);
    } else if (strcmp(cmd, "number") == 0 || strcmp(cmd, "nu") == 0) {
        state->ui.show_line_numbers = true;
        ui_set_status_message(state, "Line numbers enabled");
    } else if (strcmp(cmd, "nonumber") == 0 || strcmp(cmd, "nonu") == 0) {
        state->ui.show_line_numbers = false;
        ui_set_status_message(state, "Line numbers disabled");
    } else if (strcmp(cmd, "wq") == 0) {
        if (cmd_save_execute(state)) {
            if (state->has_stash) {
                buffer_free(&state->buffer);
                int current_rows = state->view.screen_rows;
                int current_cols = state->view.screen_cols;
                state->buffer = state->stashed_buffer;
                state->view = state->stashed_view;
                state->view.screen_rows = current_rows;
                state->view.screen_cols = current_cols;
                state->has_stash = false;
                state->editor.mode = MODE_NORMAL;
                ui_handle_resize(state);
                ui_set_status_message(state, "Returned from tutor (saved)");
                return;
            }
            exit(0);
        }
    } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "tutor") == 0) {
        if (state->has_stash) {
            ui_set_status_message(state, "Already in tutor mode");
            return;
        }
        
        state->stashed_buffer = state->buffer;
        state->stashed_view = state->view;
        state->has_stash = true;
        
        memset(&state->buffer, 0, sizeof(StitchBuffer));
        state->buffer.filename = editorStrdup("Stitch-Tutor.txt");
        
        for (int i = 0; tutor_text[i] != NULL; i++) {
            buffer_insert_line(&state->buffer, state->buffer.num_lines, (char *)tutor_text[i], strlen(tutor_text[i]));
        }
        
        state->view.cx = 0;
        state->view.cy = 0;
        state->view.row_off = 0;
        state->view.col_off = 0;
        state->view.rx = 0;
        
        state->buffer.dirty = false;
        ui_set_status_message(state, "Welcome to the Stitch Tutor");
    } else if (isdigit((unsigned char)cmd[0])) {
        int line_no = atoi(cmd);
        if (line_no < 1) line_no = 1;
        if (line_no > (int)state->buffer.num_lines) line_no = (int)state->buffer.num_lines;
        if (state->buffer.num_lines > 0) {
            state->view.cy = line_no - 1;
            state->view.cx = 0;
            ui_set_status_message(state, "Jumped to line %d", line_no);
        }
    } else if (strncmp(cmd, "s/", 2) == 0) {
        if (state->view.cy >= state->buffer.num_lines) return;
        const char *p = cmd + 2;
        const char *slash = strchr(p, '/');
        if (!slash) {
            ui_set_status_message(state, "Invalid substitution. Use :s/old/new/");
            return;
        }
        size_t old_len = slash - p;
        const char *new_str = slash + 1;
        const char *slash2 = strchr(new_str, '/');
        size_t new_len = slash2 ? (size_t)(slash2 - new_str) : strlen(new_str);
        
        if (old_len == 0) {
            ui_set_status_message(state, "Empty search string");
            return;
        }
        
        char old_buf[256];
        if (old_len >= sizeof(old_buf)) old_len = sizeof(old_buf) - 1;
        strncpy(old_buf, p, old_len);
        old_buf[old_len] = '\0';
        
        char new_buf[256];
        if (new_len >= sizeof(new_buf)) new_len = sizeof(new_buf) - 1;
        strncpy(new_buf, new_str, new_len);
        new_buf[new_len] = '\0';
        
        Line *line = &state->buffer.lines[state->view.cy];
        char *match = strstr(line->chars, old_buf);
        if (match) {
            size_t match_idx = match - line->chars;
            state->buffer.group_undo = true;
            state->view.cx = match_idx + old_len;
            for (size_t i = 0; i < old_len; i++) {
                buffer_del_char(&state->buffer, &state->view);
            }
            for (size_t i = 0; i < new_len; i++) {
                buffer_insert_char(&state->buffer, &state->view, new_buf[i]);
            }
            state->buffer.group_undo = false;
        } else {
            ui_set_status_message(state, "Pattern not found: %s", old_buf);
        }
    } else if (strncmp(cmd, "e ", 2) == 0) {
        if (state->buffer.dirty) {
            ui_set_status_message(state, "No write since last change (add ! to override)");
            return;
        }
        const char *arg = cmd + 2;
        while (*arg == ' ') arg++;
        char *filename = editorStrdup(arg);
        if (editorOpen(state, filename) == -1) {
            ui_set_status_message(state, "Could not open file: %s", filename);
        }
        free(filename);
    } else {
        ui_set_status_message(state, "Unknown command: %s", cmd);
    }
}

void editor_update_shell_status(StitchState *state) {
    if (state->core.shell_pid == -1) return;

    int status;
    pid_t result = waitpid(state->core.shell_pid, &status, WNOHANG);
    
    if (result > 0) {
        if (WIFEXITED(status)) {
            ui_set_status_message(state, "Process exited with code %d", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            ui_set_status_message(state, "Process killed by signal %d", WTERMSIG(status));
        } else {
            ui_set_status_message(state, "Process finished");
        }
        state->core.shell_pid = -1;
    } else if (result == -1) {
        if (errno == EINTR) return;
        if (errno != ECHILD) {
            ui_set_status_message(state, "Waitpid error: %s", strerror(errno));
        }
        state->core.shell_pid = -1;
    }
}
