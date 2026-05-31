#include <stdlib.h>
#include <string.h>
#include "stitch/types.h"
#include "stitch/buffer/engine.h"
#include "stitch/core/terminal.h"
#include "buffer_internal.h"

void buffer_push_undo(StitchBuffer *buf, UndoActionType type, size_t cy, size_t cx, int c, const char *text, size_t len) {
    if (buf->is_undoing) return;

    UndoAction *curr = buf->undo_stack.current;
    if (curr) curr = curr->next;
    else curr = buf->undo_stack.head;
    
    while (curr) {
        UndoAction *next = curr->next;
        if (curr->text) free(curr->text);
        free(curr);
        curr = next;
    }

    if (buf->undo_stack.current) {
        buf->undo_stack.current->next = NULL;
    } else {
        buf->undo_stack.head = NULL;
    }

    unsigned int group_id = 1;
    if (buf->undo_stack.current) {
        UndoAction *prev = buf->undo_stack.current;
        bool group = buf->group_undo;
        
        if (!group) {
            if (type == UNDO_INSERT_CHAR && prev->type == UNDO_INSERT_CHAR) {
                /* Group contiguous typing, but break on space to undo word-by-word */
                if (cy == prev->cy && cx == prev->cx + 1 && prev->c != ' ' && prev->c != '\t') {
                    group = true;
                }
            } else if (type == UNDO_DELETE_CHAR && prev->type == UNDO_DELETE_CHAR) {
                /* Group contiguous deletions (backspace or del) */
                if (cy == prev->cy && (cx == prev->cx || cx == prev->cx - 1)) {
                    group = true;
                }
            }
        }
        
        group_id = group ? prev->group_id : prev->group_id + 1;
        if (group_id == 0) group_id = 1; // Prevent overflow to 0
    }

    UndoAction *action = editorMalloc(sizeof(UndoAction));
    action->type = type;
    action->cy = cy;
    action->cx = cx;
    action->c = c;
    action->len = len;
    action->group_id = group_id;
    if (text && len > 0) {
        action->text = editorMalloc(len + 1);
        memcpy(action->text, text, len);
        action->text[len] = '\0';
    } else {
        action->text = NULL;
    }

    action->next = NULL;
    action->prev = buf->undo_stack.current;
    
    if (buf->undo_stack.current) {
        buf->undo_stack.current->next = action;
    } else {
        buf->undo_stack.head = action;
    }
    buf->undo_stack.current = action;
}

void buffer_undo(StitchBuffer *buf, StitchView *view) {
    if (!buf->undo_stack.current) return;

    buf->is_undoing = true;
    unsigned int group = buf->undo_stack.current->group_id;

    while (buf->undo_stack.current && buf->undo_stack.current->group_id == group) {
        UndoAction *action = buf->undo_stack.current;

        switch (action->type) {
            case UNDO_INSERT_CHAR:
                if (action->cy < buf->num_lines) {
                    buffer_row_del_char(buf, &buf->lines[action->cy], action->cx);
                    view->cy = action->cy;
                    view->cx = action->cx;
                }
                break;
            case UNDO_DELETE_CHAR:
                if (action->cy < buf->num_lines) {
                    buffer_row_insert_char(buf, &buf->lines[action->cy], action->cx, action->c);
                    view->cy = action->cy;
                    view->cx = action->cx + 1;
                }
                break;
            case UNDO_INSERT_LINE:
                if (action->cy < buf->num_lines) {
                    buffer_del_line(buf, action->cy);
                    view->cy = action->cy > 0 ? action->cy - 1 : 0;
                    view->cx = (buf->num_lines > 0 && view->cy < buf->num_lines) ? buf->lines[view->cy].size : 0;
                }
                break;
            case UNDO_DELETE_LINE:
                buffer_insert_line(buf, action->cy, action->text, action->len);
                view->cy = action->cy;
                view->cx = 0;
                break;
            case UNDO_MERGE_LINE:
                if (action->cy < buf->num_lines && action->cy > 0) {
                    Line *prev_line = &buf->lines[action->cy - 1];
                    if (action->cx <= prev_line->size) {
                        buffer_insert_line(buf, action->cy, &prev_line->chars[action->cx], prev_line->size - action->cx);
                        /* Re-fetch after potential realloc */
                        prev_line = &buf->lines[action->cy - 1];
                        prev_line->size = action->cx;
                        prev_line->chars[prev_line->size] = '\0';
                        buffer_update_line(prev_line);
                        view->cy = action->cy;
                        view->cx = 0;
                    }
                }
                break;
            case UNDO_SPLIT_LINE:
                if (action->cy + 1 < buf->num_lines) {
                    Line *line1 = &buf->lines[action->cy];
                    Line *line2 = &buf->lines[action->cy + 1];
                    size_t needed = line1->size + line2->size + 1;
                    if (needed > line1->capacity) {
                        line1->capacity = needed;
                        line1->chars = editorRealloc(line1->chars, line1->capacity);
                    }
                    /* Re-fetch after potential realloc */
                    line1 = &buf->lines[action->cy];
                    line2 = &buf->lines[action->cy + 1];
                    memcpy(&line1->chars[line1->size], line2->chars, line2->size);
                    line1->size += line2->size;
                    line1->chars[line1->size] = '\0';
                    buffer_update_line(line1);
                    buffer_del_line(buf, action->cy + 1);
                    view->cy = action->cy;
                    view->cx = action->cx;
                }
                break;
        }

        buf->undo_stack.current = action->prev;
    }

    buf->is_undoing = false;
}

void buffer_redo(StitchBuffer *buf, StitchView *view) {
    UndoAction *first_action;
    if (buf->undo_stack.current) {
        first_action = buf->undo_stack.current->next;
    } else {
        first_action = buf->undo_stack.head;
    }

    if (!first_action) return;
    
    buf->is_undoing = true;
    unsigned int group = first_action->group_id;
    UndoAction *action = first_action;

    while (action && action->group_id == group) {
        switch (action->type) {
            case UNDO_INSERT_CHAR:
                if (action->cy < buf->num_lines) {
                    buffer_row_insert_char(buf, &buf->lines[action->cy], action->cx, action->c);
                    view->cy = action->cy;
                    view->cx = action->cx + 1;
                }
                break;
            case UNDO_DELETE_CHAR:
                if (action->cy < buf->num_lines) {
                    buffer_row_del_char(buf, &buf->lines[action->cy], action->cx);
                    view->cy = action->cy;
                    view->cx = action->cx;
                }
                break;
            case UNDO_INSERT_LINE:
                buffer_insert_line(buf, action->cy, "", 0);
                view->cy = action->cy;
                view->cx = 0;
                break;
            case UNDO_DELETE_LINE:
                if (action->cy < buf->num_lines) {
                    buffer_del_line(buf, action->cy);
                    view->cy = action->cy > 0 ? action->cy - 1 : 0;
                    view->cx = (buf->num_lines > 0 && view->cy < buf->num_lines) ? buf->lines[view->cy].size : 0;
                }
                break;
            case UNDO_MERGE_LINE:
                if (action->cy < buf->num_lines && action->cy > 0) {
                    Line *line1 = &buf->lines[action->cy - 1];
                    Line *line2 = &buf->lines[action->cy];
                    size_t needed = line1->size + line2->size + 1;
                    if (needed > line1->capacity) {
                        line1->capacity = needed;
                        line1->chars = editorRealloc(line1->chars, line1->capacity);
                    }
                    /* Re-fetch after potential realloc */
                    line1 = &buf->lines[action->cy - 1];
                    line2 = &buf->lines[action->cy];
                    memcpy(&line1->chars[line1->size], line2->chars, line2->size);
                    line1->size += line2->size;
                    line1->chars[line1->size] = '\0';
                    buffer_update_line(line1);
                    buffer_del_line(buf, action->cy);
                    view->cy = action->cy - 1;
                    view->cx = action->cx;
                }
                break;
            case UNDO_SPLIT_LINE:
                if (action->cy < buf->num_lines) {
                    Line *line = &buf->lines[action->cy];
                    if (action->cx <= line->size) {
                        buffer_insert_line(buf, action->cy + 1, &line->chars[action->cx], line->size - action->cx);
                        /* Re-fetch after potential realloc */
                        line = &buf->lines[action->cy];
                        line->size = action->cx;
                        line->chars[line->size] = '\0';
                        buffer_update_line(line);
                        view->cy = action->cy + 1;
                        view->cx = 0;
                    }
                }
                break;
        }

        buf->undo_stack.current = action;
        action = action->next;
    }

    buf->is_undoing = false;
}
