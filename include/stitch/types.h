#ifndef STITCH_TYPES_H
#define STITCH_TYPES_H

#define _POSIX_C_SOURCE 200809L
#include <termios.h>
#include <stddef.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <stdint.h>
#include <stdbool.h>
#include <ncurses.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define STITCH_VERSION "0.1.1"
#define STITCH_TAB_STOP 4

typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_COMMAND,
    MODE_VISUAL
} Mode;

enum EditorKey {
    STITCH_KEY_NONE = 0,
    STITCH_BACKSPACE = KEY_BACKSPACE,
    STITCH_ARROW_LEFT = KEY_LEFT,
    STITCH_ARROW_RIGHT = KEY_RIGHT,
    STITCH_ARROW_UP = KEY_UP,
    STITCH_ARROW_DOWN = KEY_DOWN,
    STITCH_DEL_KEY = KEY_DC,
    STITCH_HOME_KEY = KEY_HOME,
    STITCH_END_KEY = KEY_END,
    STITCH_PAGE_UP = KEY_PPAGE,
    STITCH_PAGE_DOWN = KEY_NPAGE,
    STITCH_KEY_RESIZE = KEY_RESIZE
};

typedef struct {
    size_t size;
    size_t rsize;
    size_t capacity;
    size_t rcapacity;
    char *chars;
    char *render;
} Line;

/* --- Domain Specific Structs --- */

typedef enum {
    UNDO_INSERT_CHAR,
    UNDO_DELETE_CHAR,
    UNDO_INSERT_LINE,
    UNDO_DELETE_LINE,
    UNDO_MERGE_LINE,
    UNDO_SPLIT_LINE
} UndoActionType;

typedef struct UndoAction {
    UndoActionType type;
    size_t cy;
    size_t cx;
    int c;
    char *text;
    size_t len;
    unsigned int group_id;
    struct UndoAction *prev;
    struct UndoAction *next;
} UndoAction;

typedef struct {
    UndoAction *head;
    UndoAction *current;
} UndoStack;

typedef struct {
    Line *lines;
    size_t num_lines;
    char *filename;
    int dirty;
    UndoStack undo_stack;
    bool is_undoing;
    bool group_undo;
    bool disable_update_line;
} StitchBuffer;

typedef struct {
    size_t cx, cy;
    size_t rx;
    int screen_rows;
    int screen_cols;
    size_t row_off;
    size_t col_off;
} StitchView;

typedef struct {
    char status_msg[80];
    bool show_line_numbers;
    size_t prompt_cursor;
    size_t prompt_cursor_offset;
} StitchUI;

typedef struct {
    Mode mode;
    int last_key;
    char *search_query;
    char *history[10];
    int history_count;
    size_t visual_cx;
    size_t visual_cy;
    char *clipboard;
    size_t clipboard_len;
} StitchEditor;

typedef struct {
    struct termios orig_termios;
    pid_t shell_pid;
} StitchCore;

/* --- Unified Application Context --- */

typedef struct {
    StitchBuffer buffer;
    StitchView view;
    StitchUI ui;
    StitchEditor editor;
    StitchCore core;
    
    StitchBuffer stashed_buffer;
    StitchView stashed_view;
    bool has_stash;
} StitchState;

#endif
