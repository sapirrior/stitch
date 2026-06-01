#include <stdlib.h>
#include "stitch/types.h"
#include "stitch/ui/render.h"
#include "../editor_internal.h"

#include "stitch/buffer/engine.h"

void cmd_quit_execute(StitchState *state) {
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
        ui_set_status_message(state, "Returned from tutor");
        return;
    }

    if (state->buffer.dirty) {
        ui_set_status_message(state, "No write since last change (add ! to override)");
        return;
    }
    exit(0);
}
