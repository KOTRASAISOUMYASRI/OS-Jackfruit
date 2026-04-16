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

#define STACK_SIZE (1024 * 1024)
#define MAX_CONTAINERS 10

char stack[STACK_SIZE];

// 🧱 Container structure
struct container {
    char name[50];
    int pid;
    int active;
};

struct container containers[MAX_CONTAINERS];


// 📝 Logging function
void log_event(const char *msg) {
    FILE *fp = fopen("runtime.log", "a");
    if (!fp) return;

    fprintf(fp, "%s\n", msg);
    fclose(fp);
}


// 🔍 Find container index
int find_container(char *name) {
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active && strcmp(containers[i].name, name) == 0)
            return i;
    }
    return -1;
}


// ➕ Add container
void add_container(char *name, int pid) {
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (!containers[i].active) {
            strcpy(containers[i].name, name);
            containers[i].pid = pid;
            containers[i].active = 1;
            return;
        }
    }
}


// ❌ Remove container
void remove_container(int index) {
    containers[index].active = 0;
}


// 🧱 Container process
int child_func(void *arg) {
    char **argv = (char **)arg;

    printf("🔹 Inside container\n");

    sethostname("container", 10);

    if (chroot("rootfs-alpha") != 0) {
        perror("chroot failed");
        exit(1);
    }

    chdir("/");

    execvp(argv[0], argv);

    perror("exec failed");
    return 1;
}


// 🚀 Run container
void run_container(char *name, char *program) {

    if (find_container(name) != -1) {
        printf("Container already exists!\n");
        return;
    }

    printf("🚀 Starting container: %s\n", name);

    char *args[] = { program, NULL };

    int flags = CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS;

    int pid = clone(child_func,
                    stack + STACK_SIZE,
                    flags | SIGCHLD,
                    args);

    if (pid < 0) {
        perror("clone failed");
        return;
    }

    add_container(name, pid);

    char log_msg[100];
    sprintf(log_msg, "Container %s started with PID %d", name, pid);
    log_event(log_msg);

    printf("Container %s running with PID %d\n", name, pid);
}


// 📋 List containers
void list_containers() {
    printf("\n📋 Active Containers:\n");

    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i].active) {
            printf("Name: %s | PID: %d\n",
                   containers[i].name,
                   containers[i].pid);
        }
    }
}


// 🛑 Stop container
void stop_container(char *name) {
    int index = find_container(name);

    if (index == -1) {
        printf("Container not found!\n");
        return;
    }

    int pid = containers[index].pid;

    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);

    char log_msg[100];
    sprintf(log_msg, "Container %s stopped", name);
    log_event(log_msg);

    remove_container(index);

    printf("🛑 Container %s stopped\n", name);
}


// 🎯 MAIN
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage:\n");
        printf("./engine run <name> <program>\n");
        printf("./engine list\n");
        printf("./engine stop <name>\n");
        return 1;
    }

    // RUN
    if (strcmp(argv[1], "run") == 0) {
        if (argc < 4) {
            printf("Usage: ./engine run <name> <program>\n");
            return 1;
        }
        run_container(argv[2], argv[3]);
    }

    // LIST
    else if (strcmp(argv[1], "list") == 0) {
        list_containers();
    }

    // STOP
    else if (strcmp(argv[1], "stop") == 0) {
        if (argc < 3) {
            printf("Usage: ./engine stop <name>\n");
            return 1;
        }
        stop_container(argv[2]);
    }

    else {
        printf("Invalid command\n");
    }

    return 0;
}
