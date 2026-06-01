#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/ui/render.h"
#include "stitch/editor/modes.h"
#include "editor_internal.h"

void editor_process_keypress(StitchState *state) {
    int c = core_read_key(state);

    if (c == STITCH_KEY_NONE) return;

    if (c == STITCH_KEY_RESIZE) {
        ui_handle_resize(state);
        return;
    }

    if (c == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK) {
            if (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED)) {
                if (event.y >= 0 && event.x >= 0 && event.y < state->view.screen_rows) {
                    ui_screen_to_buffer(state, event.y, event.x, &state->view.cy, &state->view.cx);
                }
            }
        }
        return;
    }

    if (state->editor.mode == MODE_NORMAL) {
        handle_normal_mode(state, c);
    } else if (state->editor.mode == MODE_INSERT) {
        handle_insert_mode(state, c);
    } else if (state->editor.mode == MODE_VISUAL) {
        handle_visual_mode(state, c);
    }
}
