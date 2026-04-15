/*
 * engine.c - Supervised Multi-Container Runtime (User Space)
 *
 * Intentionally partial starter:
 *   - command-line shape is defined
 *   - key runtime data structures are defined
 *   - bounded-buffer skeleton is defined
 *   - supervisor / client split is outlined
 *
 * Students are expected to design:
 *   - the control-plane IPC implementation
 *   - container lifecycle and metadata synchronization
 *   - clone + namespace setup for each container
 *   - producer/consumer behavior for log buffering
 *   - signal handling and graceful shutdown
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define STACK_SIZE (1024 * 1024)

char container_stack[STACK_SIZE];

/* IOCTL COMMAND */
#define IOCTL_MONITOR_PID _IOW('a', 'a', int)

/* ================= LOGGING ================= */
void log_event(const char *message) {
    FILE *f = fopen("runtime.log", "a");
    if (f == NULL) {
        perror("log file error");
        return;
    }
    fprintf(f, "%s\n", message);
    fclose(f);
}

/* ================= CHILD ================= */
int child_func(void *arg) {
    char **argv = (char **)arg;

    printf("Inside container!\n");

    sethostname("container", 10);

    if (chroot("rootfs-alpha") != 0) {
        perror("chroot failed");
        log_event("ERROR: chroot failed");
        exit(1);
    }

    chdir("/");

    execvp(argv[0], argv);

    perror("exec failed");
    log_event("ERROR: exec failed");

    return 1;
}

/* ================= MAIN ================= */
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage:\n");
        printf("  %s run <name> <program>\n", argv[0]);
        printf("  %s list\n", argv[0]);
        printf("  %s stop <name>\n", argv[0]);
        return 1;
    }

    /* ================= RUN ================= */
    if (strcmp(argv[1], "run") == 0) {
        if (argc < 4) {
            printf("Usage: %s run <name> <program>\n", argv[0]);
            return 1;
        }

        printf("Starting container: %s\n", argv[2]);

        char *child_args[] = { argv[3], NULL };

        int flags = CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS;

        int pid = clone(child_func,
                        container_stack + STACK_SIZE,
                        flags | SIGCHLD,
                        child_args);

        if (pid < 0) {
            perror("clone failed");
            log_event("ERROR: clone failed");
            return 1;
        }

        printf("Container %s started with PID %d\n", argv[2], pid);

        /* SEND PID TO KERNEL */
        int fd = open("/dev/monitor", O_RDWR);
        if (fd >= 0) {
            ioctl(fd, IOCTL_MONITOR_PID, &pid);
            close(fd);
        }

        char log_msg[200];
        sprintf(log_msg, "START: Container=%s PID=%d", argv[2], pid);
        log_event(log_msg);
    }

    /* ================= LIST ================= */
    else if (strcmp(argv[1], "list") == 0) {
        printf("Running containers:\n");
        system("ps -ef | grep hog | grep -v grep");
    }

    /* ================= STOP ================= */
    else if (strcmp(argv[1], "stop") == 0) {
        if (argc < 3) {
            printf("Usage: %s stop <name>\n", argv[0]);
            return 1;
        }

        char command[100];
        sprintf(command, "pkill -f %s", argv[2]);
        system(command);

        printf("Stopped container: %s\n", argv[2]);

        char log_msg[200];
        sprintf(log_msg, "STOP: Container=%s", argv[2]);
        log_event(log_msg);
    }

    else {
        printf("Unknown command\n");
    }

    return 0;
}
