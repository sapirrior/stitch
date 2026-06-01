#ifndef STITCH_EDITOR_INTERNAL_H
#define STITCH_EDITOR_INTERNAL_H

#include "stitch/types.h"

#include <stdbool.h>

/* Mode handlers */
void handle_normal_mode(StitchState *state, int c);
void handle_insert_mode(StitchState *state, int c);
void handle_command_prompt_mode(StitchState *state, int c);
void handle_visual_mode(StitchState *state, int c);

/* Command components */
bool cmd_save_execute(StitchState *state);
void cmd_quit_execute(StitchState *state);
void cmd_search_execute(StitchState *state);
void cmd_shell_execute(StitchState *state, const char *command);

/* Movement logic */
void editor_move_cursor(StitchState *state, int key);
void editor_move_word_forward(StitchState *state);
void editor_move_word_backward(StitchState *state);
void editor_find_char(StitchState *state, int c);

#endif
