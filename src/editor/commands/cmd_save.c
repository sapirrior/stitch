#include <stdlib.h>
#include "stitch/types.h"
#include "stitch/buffer/io.h"
#include "stitch/ui/render.h"
#include "../editor_internal.h"

bool cmd_save_execute(StitchState *state) {
    return editorSave(state);
}
