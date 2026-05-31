#include <ncurses.h>
#include <unistd.h>
#include "stitch/core/terminal.h"
#include "core_internal.h"

void core_enable_raw_mode(StitchState *state) {
    sys_init_ncurses(state);
}

void core_disable_raw_mode(void) {
    /* Re-enable Bracketed Paste Mode on exit so shell continues to behave correctly */
    printf("\x1b[?2004h");
    fflush(stdout);
    if (!isendwin()) endwin();
}

int core_read_key(StitchState *state) {
    (void)state;
    static int last_c = 0;
    int c = getch();
    if (c == ERR) return STITCH_KEY_NONE;
    
    if (c == '\n' && last_c == '\r') {
        last_c = c;
        return STITCH_KEY_NONE;
    }
    last_c = c;

    if (c == KEY_ENTER || c == '\n' || c == '\r') return '\r';
    if (c == 127 || c == KEY_BACKSPACE) return STITCH_BACKSPACE;
    if (c < 32) return c;
    
    switch (c) {
        case KEY_LEFT: return STITCH_ARROW_LEFT;
        case KEY_RIGHT: return STITCH_ARROW_RIGHT;
        case KEY_UP: return STITCH_ARROW_UP;
        case KEY_DOWN: return STITCH_ARROW_DOWN;
        case KEY_DC: return STITCH_DEL_KEY;
        case KEY_HOME: return STITCH_HOME_KEY;
        case KEY_END: return STITCH_END_KEY;
        case KEY_PPAGE: return STITCH_PAGE_UP;
        case KEY_NPAGE: return STITCH_PAGE_DOWN;
        case KEY_RESIZE: return STITCH_KEY_RESIZE;
    }
    return c;
}

int core_get_window_size(int *rows, int *cols) {
    getmaxyx(stdscr, *rows, *cols);
    return 0;
}

int core_is_utf8_start(unsigned char c) {
    return (c < 0x80) || (c >= 0xc0);
}

int editorIsUtf8Start(unsigned char c) { return core_is_utf8_start(c); }

size_t editorRowByteToCol(const char *s, size_t len, size_t target_byte) {
    size_t col = 0;
    for (size_t i = 0; i < target_byte && i < len; i++) {
        if (core_is_utf8_start((unsigned char)s[i])) col++;
    }
    return col;
}

size_t editorRowColToByte(const char *s, size_t len, size_t target_col) {
    size_t byte = 0;
    size_t col = 0;
    while (byte < len && col < target_col) {
        if (core_is_utf8_start((unsigned char)s[byte])) col++;
        byte++;
    }
    while (byte < len && !core_is_utf8_start((unsigned char)s[byte])) byte++;
    return byte;
}
