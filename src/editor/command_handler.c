#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/io.h"
#include "stitch/ui/render.h"
#include "editor_internal.h"

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
            exit(0);
        }
    } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
        state->ui.show_help_overlay = true;
    } else if (strncmp(cmd, "e ", 2) == 0) {
        if (state->buffer.dirty) {
            ui_set_status_message(state, "No write since last change (add ! to override)");
            return;
        }
        char *filename = editorStrdup(cmd + 2);
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
