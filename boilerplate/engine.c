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

/* ================= STRUCTS ================= */

typedef enum {
    CMD_START, CMD_PS, CMD_STOP
} command_kind_t;

typedef struct {
    command_kind_t kind;
    char id[32];
    char rootfs[128];
    char command[128];
} control_request_t;

typedef struct {
    char id[32];
    pid_t pid;
    char log_path[PATH_MAX];
    struct container *next;
} container_t;

typedef struct {
    char id[32];
    size_t len;
    char data[LOG_CHUNK_SIZE];
} log_item_t;

typedef struct {
    log_item_t items[LOG_BUFFER_CAPACITY];
    int head, tail, count, shutdown;
    pthread_mutex_t lock;
    pthread_cond_t not_empty, not_full;
} buffer_t;

/* ================= GLOBAL ================= */

container_t *head = NULL;
pthread_mutex_t container_lock;

buffer_t buf;

/* ================= BUFFER ================= */

void buffer_init(buffer_t *b) {
    memset(b, 0, sizeof(*b));
    pthread_mutex_init(&b->lock, NULL);
    pthread_cond_init(&b->not_empty, NULL);
    pthread_cond_init(&b->not_full, NULL);
}

void buffer_shutdown(buffer_t *b) {
    pthread_mutex_lock(&b->lock);
    b->shutdown = 1;
    pthread_cond_broadcast(&b->not_empty);
    pthread_cond_broadcast(&b->not_full);
    pthread_mutex_unlock(&b->lock);
}

int buffer_push(buffer_t *b, log_item_t *item) {
    pthread_mutex_lock(&b->lock);
    while (b->count == LOG_BUFFER_CAPACITY && !b->shutdown)
        pthread_cond_wait(&b->not_full, &b->lock);

    if (b->shutdown) {
        pthread_mutex_unlock(&b->lock);
        return -1;
    }

    b->items[b->tail] = *item;
    b->tail = (b->tail + 1) % LOG_BUFFER_CAPACITY;
    b->count++;

    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->lock);
    return 0;
}

int buffer_pop(buffer_t *b, log_item_t *item) {
    pthread_mutex_lock(&b->lock);
    while (b->count == 0 && !b->shutdown)
        pthread_cond_wait(&b->not_empty, &b->lock);

    if (b->count == 0 && b->shutdown) {
        pthread_mutex_unlock(&b->lock);
        return -1;
    }

    *item = b->items[b->head];
    b->head = (b->head + 1) % LOG_BUFFER_CAPACITY;
    b->count--;

    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->lock);
    return 0;
}

/* ================= LOGGING ================= */

void *logger(void *arg) {
    log_item_t item;

    mkdir(LOG_DIR, 0777);

    while (buffer_pop(&buf, &item) == 0) {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s.log", LOG_DIR, item.id);

        FILE *f = fopen(path, "a");
        if (f) {
            fwrite(item.data, 1, item.len, f);
            fclose(f);
        }
    }
    return NULL;
}

/* ================= CHILD ================= */

int child_fn(void *arg) {
    char **cmd = (char **)arg;

    sethostname("container", 10);
    chroot("rootfs");   // simplified
    chdir("/");
    mount("proc", "/proc", "proc", 0, NULL);

    execvp(cmd[0], cmd);
    perror("exec");
    return 1;
}

/* ================= CONTAINER ================= */

void add_container(char *id, pid_t pid) {
    pthread_mutex_lock(&container_lock);

    container_t *c = malloc(sizeof(container_t));
    strcpy(c->id, id);
    c->pid = pid;

    c->next = head;
    head = c;

    pthread_mutex_unlock(&container_lock);
}

container_t* find_container(char *id) {
    container_t *c = head;
    while (c) {
        if (strcmp(c->id, id) == 0)
            return c;
        c = c->next;
    }
    return NULL;
}

/* ================= SUPERVISOR ================= */

void run_supervisor() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    pthread_t log_thread;

    buffer_init(&buf);
    pthread_create(&log_thread, NULL, logger, NULL);

    unlink(CONTROL_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CONTROL_PATH);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    printf("Supervisor running...\n");

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);

        control_request_t req;
        read(client_fd, &req, sizeof(req));

        if (req.kind == CMD_START) {
            char *args[] = {req.command, NULL};

            char stack[STACK_SIZE];
            int pid = clone(child_fn,
                            stack + STACK_SIZE,
                            CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                            args);

            add_container(req.id, pid);
            write(client_fd, "Started\n", 8);
        }

        else if (req.kind == CMD_PS) {
            container_t *c = head;
            char out[128];

            while (c) {
                sprintf(out, "%s : PID %d\n", c->id, c->pid);
                write(client_fd, out, strlen(out));
                c = c->next;
            }
        }

        else if (req.kind == CMD_STOP) {
            container_t *c = find_container(req.id);
            if (c) {
                kill(c->pid, SIGKILL);
                write(client_fd, "Stopped\n", 8);
            }
        }

        close(client_fd);
    }
}

/* ================= CLIENT ================= */

void send_req(control_request_t *req) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, CONTROL_PATH);

    connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    write(fd, req, sizeof(*req));

    char buf[256];
    int n = read(fd, buf, sizeof(buf)-1);
    buf[n] = 0;
    printf("%s", buf);

    close(fd);
}

/* ================= MAIN ================= */

int main(int argc, char *argv[]) {

    pthread_mutex_init(&container_lock, NULL);

    if (argc < 2) return 1;

    if (!strcmp(argv[1], "supervisor")) {
        run_supervisor();
    }

    else if (!strcmp(argv[1], "start")) {
        control_request_t req = {CMD_START};
        strcpy(req.id, argv[2]);
        strcpy(req.command, argv[3]);
        send_req(&req);
    }

    else if (!strcmp(argv[1], "ps")) {
        control_request_t req = {CMD_PS};
        send_req(&req);
    }

    else if (!strcmp(argv[1], "stop")) {
        control_request_t req = {CMD_STOP};
        strcpy(req.id, argv[2]);
        send_req(&req);
    }

    return 0;
}
