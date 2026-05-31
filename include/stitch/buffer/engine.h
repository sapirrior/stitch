#ifndef STITCH_BUFFER_ENGINE_H
#define STITCH_BUFFER_ENGINE_H

#include "stitch/types.h"

void buffer_insert_line(StitchBuffer *buf, size_t at, char *s, size_t len);
void buffer_del_line(StitchBuffer *buf, size_t at);
void buffer_free(StitchBuffer *buf);
void buffer_update_line(Line *line);

void buffer_insert_char(StitchBuffer *buf, StitchView *view, int c);
void buffer_insert_newline(StitchBuffer *buf, StitchView *view);
void buffer_del_char(StitchBuffer *buf, StitchView *view);

void buffer_push_undo(StitchBuffer *buf, UndoActionType type, size_t cy, size_t cx, int c, const char *text, size_t len);
void buffer_undo(StitchBuffer *buf, StitchView *view);
void buffer_redo(StitchBuffer *buf, StitchView *view);

#endif
