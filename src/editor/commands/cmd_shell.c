#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "stitch/types.h"
#include "stitch/ui/render.h"
#include "../editor_internal.h"

void cmd_shell_execute(StitchState *state, const char *cmd) {
    if (state->core.shell_pid != -1) {
        ui_set_status_message(state, "A process is already running (PID %d)", state->core.shell_pid);
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        ui_set_status_message(state, "Fork failed: %s", strerror(errno));
        return;
    }

    if (pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        } else {
            exit(127);
        }
        
        char *args[] = {"sh", "-c", (char *)cmd, NULL};
        execvp("sh", args);
        exit(1);
    } else {
        state->core.shell_pid = pid;
        ui_set_status_message(state, "Running: %s...", cmd);
    }
}
