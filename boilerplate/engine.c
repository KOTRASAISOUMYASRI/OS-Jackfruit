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
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024)
#define CONTROL_PATH "/tmp/mini_runtime.sock"
#define LOG_DIR "logs"
#define LOG_BUFFER_CAPACITY 16
#define LOG_CHUNK_SIZE 4096

/* ===================== STRUCTS ===================== */

typedef struct {
    char container_id[32];
    size_t length;
    char data[LOG_CHUNK_SIZE];
} log_item_t;

typedef struct {
    log_item_t items[LOG_BUFFER_CAPACITY];
    int head, tail, count;
    int shutting_down;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} bounded_buffer_t;

typedef struct container {
    char id[32];
    pid_t pid;
    char log_path[PATH_MAX];
    struct container *next;
} container_t;

/* ===================== GLOBAL ===================== */

bounded_buffer_t buffer;
container_t *containers = NULL;
pthread_mutex_t container_lock;
int running = 1;

/* ===================== BUFFER ===================== */

int bounded_buffer_push(bounded_buffer_t *b, const log_item_t *item) {
    pthread_mutex_lock(&b->mutex);

    while (b->count == LOG_BUFFER_CAPACITY && !b->shutting_down)
        pthread_cond_wait(&b->not_full, &b->mutex);

    if (b->shutting_down) {
        pthread_mutex_unlock(&b->mutex);
        return -1;
    }

    b->items[b->tail] = *item;
    b->tail = (b->tail + 1) % LOG_BUFFER_CAPACITY;
    b->count++;

    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
    return 0;
}

int bounded_buffer_pop(bounded_buffer_t *b, log_item_t *item) {
    pthread_mutex_lock(&b->mutex);

    while (b->count == 0 && !b->shutting_down)
        pthread_cond_wait(&b->not_empty, &b->mutex);

    if (b->count == 0 && b->shutting_down) {
        pthread_mutex_unlock(&b->mutex);
        return -1;
    }

    *item = b->items[b->head];
    b->head = (b->head + 1) % LOG_BUFFER_CAPACITY;
    b->count--;

    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return 0;
}

/* ===================== LOGGING ===================== */

void *logging_thread(void *arg) {
    (void)arg;
    log_item_t item;

    while (bounded_buffer_pop(&buffer, &item) == 0) {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s.log", LOG_DIR, item.container_id);

        FILE *f = fopen(path, "a");
        if (f) {
            fwrite(item.data, 1, item.length, f);
            fclose(f);
        }
    }
    return NULL;
}

/* ===================== CHILD ===================== */

int child_fn(void *arg) {
    char **cmd = (char **)arg;

    sethostname("container", 10);

    chroot("rootfs-alpha");
    chdir("/");

    mount("proc", "/proc", "proc", 0, NULL);

    execvp(cmd[0], cmd);

    perror("exec failed");
    return 1;
}

/* ===================== CONTAINER MGMT ===================== */

void add_container(const char *id, pid_t pid) {
    pthread_mutex_lock(&container_lock);

    container_t *c = malloc(sizeof(container_t));
    strcpy(c->id, id);
    c->pid = pid;
    snprintf(c->log_path, sizeof(c->log_path), "%s/%s.log", LOG_DIR, id);

    c->next = containers;
    containers = c;

    pthread_mutex_unlock(&container_lock);
}

container_t *find_container(const char *id) {
    container_t *c = containers;
    while (c) {
        if (strcmp(c->id, id) == 0)
            return c;
        c = c->next;
    }
    return NULL;
}

/* ===================== SIGNAL ===================== */

void handle_sig(int sig) {
    (void)sig;
    running = 0;
    buffer.shutting_down = 1;
    pthread_cond_broadcast(&buffer.not_empty);
    pthread_cond_broadcast(&buffer.not_full);
}

/* ===================== SUPERVISOR ===================== */

void run_supervisor() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    pthread_t log_thread;

    mkdir(LOG_DIR, 0777);

    pthread_mutex_init(&buffer.mutex, NULL);
    pthread_cond_init(&buffer.not_empty, NULL);
    pthread_cond_init(&buffer.not_full, NULL);

    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    pthread_create(&log_thread, NULL, logging_thread, NULL);

    unlink(CONTROL_PATH);
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CONTROL_PATH);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Supervisor running...\n");

    while (running) {
        client_fd = accept(server_fd, NULL, NULL);

        char buf[256] = {0};
        read(client_fd, buf, sizeof(buf));

        char cmd[32], id[32], prog[128];
        sscanf(buf, "%s %s %s", cmd, id, prog);

        if (strcmp(cmd, "start") == 0) {
            char *args[] = {prog, NULL};

            char stack[STACK_SIZE];
            int pid = clone(child_fn,
                            stack + STACK_SIZE,
                            CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                            args);

            add_container(id, pid);

            write(client_fd, "Started\n", 8);
        }

        else if (strcmp(cmd, "ps") == 0) {
            container_t *c = containers;
            char out[256];

            while (c) {
                sprintf(out, "%s : PID %d\n", c->id, c->pid);
                write(client_fd, out, strlen(out));
                c = c->next;
            }
        }

        else if (strcmp(cmd, "stop") == 0) {
            container_t *c = find_container(id);
            if (c) {
                kill(c->pid, SIGKILL);
                write(client_fd, "Stopped\n", 8);
            }
        }

        close(client_fd);
    }

    pthread_join(log_thread, NULL);
    close(server_fd);
}

/* ===================== CLIENT ===================== */

void send_cmd(const char *msg) {
    int sock;
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CONTROL_PATH);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    write(sock, msg, strlen(msg));

    char buf[256];
    int n = read(sock, buf, sizeof(buf) - 1);
    buf[n] = 0;
    printf("%s", buf);

    close(sock);
}

/* ===================== MAIN ===================== */

int main(int argc, char *argv[]) {

    pthread_mutex_init(&container_lock, NULL);

    if (argc < 2) {
        printf("Usage: supervisor | start | ps | stop\n");
        return 1;
    }

    if (strcmp(argv[1], "supervisor") == 0) {
        run_supervisor();
    }

    else if (strcmp(argv[1], "start") == 0) {
        char msg[256];
        sprintf(msg, "start %s %s", argv[2], argv[3]);
        send_cmd(msg);
    }

    else if (strcmp(argv[1], "ps") == 0) {
        send_cmd("ps");
    }

    else if (strcmp(argv[1], "stop") == 0) {
        char msg[256];
        sprintf(msg, "stop %s", argv[2]);
        send_cmd(msg);
    }

    return 0;
}
