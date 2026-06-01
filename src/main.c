#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <ncurses.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/io.h"
#include "stitch/ui/render.h"
#include "stitch/editor/modes.h"
#include "stitch/editor/commands.h"

void app_at_exit(void) {
    core_disable_raw_mode();
}

void init_app_state(StitchState *state) {
    state->buffer.lines = NULL;
    state->buffer.num_lines = 0;
    state->buffer.filename = NULL;
    state->buffer.dirty = 0;
    state->buffer.undo_stack.head = NULL;
    state->buffer.undo_stack.current = NULL;
    state->buffer.is_undoing = false;
    state->buffer.group_undo = false;
    state->buffer.disable_update_line = false;

    state->view.cx = 0;
    state->view.cy = 0;
    state->view.row_off = 0;
    state->view.col_off = 0;

    state->ui.status_msg[0] = '\0';
    state->ui.show_line_numbers = false;
    state->ui.prompt_cursor = 0;
    state->ui.prompt_cursor_offset = 0;

    state->editor.mode = MODE_NORMAL;
    state->editor.last_key = 0;
    state->editor.search_query = NULL;
    state->editor.history_count = 0;
    for (int i = 0; i < 10; i++) state->editor.history[i] = NULL;

    state->editor.clipboard = NULL;
    state->editor.clipboard_len = 0;

    state->core.shell_pid = -1;

    core_get_window_size(&state->view.screen_rows, &state->view.screen_cols);
    state->view.screen_rows -= 2; // Reserve for status bar and message bar
    if (state->view.screen_rows < 0) state->view.screen_rows = 0;
    if (state->view.screen_cols < 1) state->view.screen_cols = 1;
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    
    StitchState app;
    atexit(app_at_exit);
    core_enable_raw_mode(&app);
    init_app_state(&app);

    if (argc >= 2) {
        if (editorOpen(&app, argv[1]) == -1) {
            ui_set_status_message(&app, "Could not open file: %s", argv[1]);
        }
    }

    ui_set_status_message(&app, ":h for help");

    while (1) {
        if (app.core.shell_pid != -1) editor_update_shell_status(&app);
        ui_refresh_screen(&app);
        editor_process_keypress(&app);
    }

    return 0;
}
