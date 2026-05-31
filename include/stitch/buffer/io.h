#ifndef STITCH_BUFFER_IO_H
#define STITCH_BUFFER_IO_H

#include <stdbool.h>
#include "stitch/types.h"

bool editorSave(StitchState *state);
int editorOpen(StitchState *state, char *filename);

#endif
