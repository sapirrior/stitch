#ifndef STITCH_BUFFER_INTERNAL_H
#define STITCH_BUFFER_INTERNAL_H

#include "stitch/types.h"

/* Internal line management */
void buffer_row_insert_char(StitchBuffer *buf, Line *line, size_t at, int c);
void buffer_row_del_char(StitchBuffer *buf, Line *line, size_t at);
void buffer_insert_line(StitchBuffer *buf, size_t at, char *s, size_t len);
void buffer_del_line(StitchBuffer *buf, size_t at);
void buffer_free_line(Line *line);

#endif
