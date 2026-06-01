#include <stdlib.h>
#include <string.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "stitch/buffer/engine.h"
#include "buffer_internal.h"

void buffer_row_insert_char(StitchBuffer *buf, Line *line, size_t at, int c) {
    if (at > line->size) at = line->size;
    
    if (line->size + 1 >= line->capacity) {
        if (line->capacity == 0) line->capacity = 16;
        else line->capacity *= 2;
        line->chars = editorRealloc(line->chars, line->capacity);
    }
    
    memmove(&line->chars[at + 1], &line->chars[at], line->size - at + 1);
    line->size++;
    line->chars[at] = c;
    if (!buf->disable_update_line) buffer_update_line(line);
    buf->dirty++;
}

void buffer_insert_char(StitchBuffer *buf, StitchView *view, int c) {
    if (view->cy > buf->num_lines) return;

    if (view->cy == buf->num_lines) {
        buffer_insert_line(buf, buf->num_lines, "", 0);
        buffer_push_undo(buf, UNDO_INSERT_LINE, buf->num_lines - 1, 0, 0, NULL, 0);
    }
    
    if (view->cy < buf->num_lines) {
        buffer_push_undo(buf, UNDO_INSERT_CHAR, view->cy, view->cx, c, NULL, 0);
        buffer_row_insert_char(buf, &buf->lines[view->cy], view->cx, c);
        view->cx++;
    }
}

void buffer_insert_newline(StitchBuffer *buf, StitchView *view) {
    if (view->cy > buf->num_lines) return;

    if (view->cy == buf->num_lines) {
        buffer_insert_line(buf, buf->num_lines, "", 0);
        buffer_push_undo(buf, UNDO_INSERT_LINE, buf->num_lines - 1, 0, 0, NULL, 0);
        view->cy++;
        view->cx = 0;
        return;
    }

    Line *line = &buf->lines[view->cy];
    if (view->cx == 0) {
        buffer_insert_line(buf, view->cy, "", 0);
        buffer_push_undo(buf, UNDO_INSERT_LINE, view->cy, 0, 0, NULL, 0);
    } else {
        if (view->cx > line->size) view->cx = line->size;
        
        buffer_push_undo(buf, UNDO_SPLIT_LINE, view->cy, view->cx, 0, NULL, 0);
        buffer_insert_line(buf, view->cy + 1, &line->chars[view->cx], line->size - view->cx);
        
        /* The buffer_insert_line might have reallocated buf->lines, 
         * so we MUST re-fetch the pointer to the current line. */
        line = &buf->lines[view->cy];
        line->size = view->cx;
        line->chars[line->size] = '\0';
        if (!buf->disable_update_line) buffer_update_line(line);
    }
    view->cy++;
    view->cx = 0;
}
