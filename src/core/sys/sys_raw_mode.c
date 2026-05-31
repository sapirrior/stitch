#include <ncurses.h>
#include "stitch/types.h"
#include "../core_internal.h"

void sys_init_colors(void) {
    start_color();
    use_default_colors();

    /* Define Organic Warmth Palette for functional highlights */
    if (can_change_color()) {
        init_color(10, 580, 680, 560); /* Sage Green */
        init_color(11, 840, 580, 480); /* Terracotta Orange */
        init_color(12, 940, 780, 380); /* Ochre Yellow */
        init_color(13, 184, 165, 157); /* Earth Background */
        init_color(14, 600, 400, 600); /* Muted Plum */
    }

    /* 1: Earth on Sage (Normal Mode) */
    init_pair(1, 13, 10);
    /* 2: Earth on Terracotta (Insert Mode) */
    init_pair(2, 13, 11);
    /* 3: Earth on Ochre (Command Mode) */
    init_pair(3, 13, 12);
    /* 4: Standard Text */
    init_pair(4, -1, -1);
    /* 5: Earth on Sage (Position Indicator) */
    init_pair(5, 13, 10);
    /* 6: Earth Background for Status Bar */
    init_pair(6, -1, 13);
    /* 7: Earth on Plum (Visual Mode) */
    init_pair(7, 13, 14);
    
    bkgd(COLOR_PAIR(4));
}

void sys_init_ncurses(StitchState *state) {
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    set_escdelay(50);
    mousemask(BUTTON1_CLICKED | BUTTON1_PRESSED, NULL);
    start_color();
    sys_init_colors();
    
    /* Disable Bracketed Paste Mode on startup so terminal pasted text isn't intercepted as Escape sequences */
    printf("\x1b[?2004l");
    fflush(stdout);
    
    (void)state;
}
