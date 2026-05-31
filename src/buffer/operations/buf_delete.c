#include <stdlib.h>
#include <string.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/engine.h"
#include "buffer_internal.h"

void buffer_row_del_char(StitchBuffer *buf, Line *line, size_t at) {
    if (at >= line->size) return;
    memmove(&line->chars[at], &line->chars[at + 1], line->size - at);
    line->size--;
    if (!buf->disable_update_line) buffer_update_line(line);
    buf->dirty++;
}

void buffer_del_char(StitchBuffer *buf, StitchView *view) {
    if (view->cy >= buf->num_lines) return;
    if (view->cx == 0 && view->cy == 0) return;

    Line *line = &buf->lines[view->cy];
    if (view->cx > 0) {
        if (view->cx > line->size) view->cx = line->size;

        size_t bytes_to_del = 1;
        /* Handle multibyte UTF-8 characters: delete all continuation bytes */
        while (view->cx - bytes_to_del > 0 && (line->chars[view->cx - bytes_to_del] & 0xc0) == 0x80) {
            bytes_to_del++;
        }

        while (bytes_to_del--) {
            int c = (unsigned char)line->chars[view->cx - 1];
            buffer_push_undo(buf, UNDO_DELETE_CHAR, view->cy, view->cx - 1, c, NULL, 0);
            buffer_row_del_char(buf, line, view->cx - 1);
            view->cx--;
        }
    } else {
        size_t prev_line_idx = view->cy - 1;
        view->cx = buf->lines[prev_line_idx].size;

        buffer_push_undo(buf, UNDO_MERGE_LINE, view->cy, view->cx, 0, NULL, 0);

        size_t prev_len = buf->lines[prev_line_idx].size;
        size_t needed = prev_len + line->size + 1;

        if (needed > buf->lines[prev_line_idx].capacity) {
            buf->lines[prev_line_idx].capacity = needed;
            buf->lines[prev_line_idx].chars = editorRealloc(buf->lines[prev_line_idx].chars, buf->lines[prev_line_idx].capacity);
        }

        /* Re-fetch line pointer as realloc might have moved buf->lines */
        line = &buf->lines[view->cy];
        memcpy(&buf->lines[prev_line_idx].chars[prev_len], line->chars, line->size);
        buf->lines[prev_line_idx].size += line->size;
        buf->lines[prev_line_idx].chars[buf->lines[prev_line_idx].size] = '\0';
        buffer_update_line(&buf->lines[prev_line_idx]);
        buffer_del_line(buf, view->cy);
        view->cy--;
    }
}
