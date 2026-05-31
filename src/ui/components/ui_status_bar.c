#include <ncurses.h>
#include <string.h>
#include "stitch/types.h"
#include "stitch/core/terminal.h"
#include "ui_internal.h"

void ui_status_bar_draw(StitchState *state) {
    int mode_pair = 1;
    const char *mode_str = " NORMAL ";
    if (state->editor.mode == MODE_INSERT) { mode_pair = 2; mode_str = " INSERT "; }
    else if (state->editor.mode == MODE_COMMAND) { mode_pair = 3; mode_str = " COMMAND "; }
    else if (state->editor.mode == MODE_VISUAL) { mode_pair = 7; mode_str = " VISUAL "; }

    int y = state->view.screen_rows;
    move(y, 0);
    
    attron(COLOR_PAIR(mode_pair));
    addstr(mode_str);
    attroff(COLOR_PAIR(mode_pair));

    attron(COLOR_PAIR(6));
    char status[120], rstatus[120];
    int len = snprintf(status, sizeof(status), " %s%s",
        state->buffer.filename ? state->buffer.filename : "[No Name]",
        state->buffer.dirty ? "*" : "");
    if (len < 0) len = 0;
    snprintf(rstatus, sizeof(rstatus), " %zu:%zu ",
        state->view.cy + 1, state->view.cx + 1);

    int mode_len = (int)strlen(mode_str);
    /* Fill remaining space with background */
    for (int i = mode_len; i < state->view.screen_cols; i++) addch(' ');

    move(y, mode_len);
    int avail = state->view.screen_cols - mode_len - (int)strlen(rstatus) - 1;
    if (avail < 0) avail = 0;
    int status_bytes = editorRowColToByte(status, len, (size_t)avail);
    addnstr(status, status_bytes);
    
    int rstatus_len = (int)strlen(rstatus);
    move(y, state->view.screen_cols - rstatus_len);
    attron(COLOR_PAIR(5));
    addstr(rstatus);
    attroff(COLOR_PAIR(5));
    attroff(COLOR_PAIR(6));
}
