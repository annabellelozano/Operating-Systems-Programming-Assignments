//Server code for Programming Assignment 1
//By: Annabelle Lozano
//Due: September 25th, 2025

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SERVER_FIFO "./server_fifo"
#define MAX_PARAMS_LEN 512
#define MAX_CLIENTS 128
#define MAX_FIFO_NAME 128

typedef struct {
    pid_t pid;
    int syscall_num;
    int num_params;
    int param_size;        // Total bytes of parameters[]
    char params[MAX_PARAMS_LEN]; // Parameters encoded as "p1|p2|p3"
} Request;

typedef struct {
    pid_t pid;
    int result_size;
    char result[512];
} Reply;

// Map to remember client pid -> Write fd and fifo name 
typedef struct {
    pid_t pid;
    int fd;                 // fd opened to client fifo (O_WRONLY)
    char fifo_name[MAX_FIFO_NAME];
} ClientInfo;

static ClientInfo clients[MAX_CLIENTS];

static int find_client_index(pid_t pid) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].pid == pid) return i;
    }
    return -1;
}
// Add client
static int add_client(pid_t pid, const char *fifo_name, int fd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].pid == 0) {
            clients[i].pid = pid;
            clients[i].fd = fd;
            strncpy(clients[i].fifo_name, fifo_name, MAX_FIFO_NAME-1);
            clients[i].fifo_name[MAX_FIFO_NAME-1] = '\0';
            return i;
        }
    }
    return -1; // full
}
//Remove clident index
static void remove_client_index(int idx) {
    if (idx < 0 || idx >= MAX_CLIENTS) return;
    if (clients[idx].fd > 0) close(clients[idx].fd);
    clients[idx].pid = 0;
    clients[idx].fd = -1;
    clients[idx].fifo_name[0] = '\0';
}

static void cleanup_all_clients() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].pid != 0) {
            if (clients[i].fd > 0) close(clients[i].fd);
            clients[i].pid = 0;
            clients[i].fd = -1;
            clients[i].fifo_name[0] = '\0';
        }
    }
}

int main(void) {
    int server_fd;
    Request req;
    Reply rep;
    int stored_value = 0; // This is for syscall 4 and 5

    // Initialise client map 
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].pid = 0;
        clients[i].fd = -1;
        clients[i].fifo_name[0] = '\0';
    }

    // Create well-known FIFO 
    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo(server_fifo)");
        exit(EXIT_FAILURE);
    }

    printf("Server: starting up, creating/opening %s\n", SERVER_FIFO);

    // Open server FIFO read/write 
    server_fd = open(SERVER_FIFO, O_RDWR);
    if (server_fd == -1) {
        perror("open(server_fifo)");
        unlink(SERVER_FIFO);
        exit(EXIT_FAILURE);
    }

    printf("Server: ready to receive requests.\n");

    while (1) {
        ssize_t r = read(server_fd, &req, sizeof(Request));
        if (r == -1) {
            if (errno == EINTR) continue;
            perror("read(server_fifo)");
            break;
        } else if (r == 0) {
            // no data; continue 
            continue;
        } else if (r < (ssize_t)sizeof(Request)) {
            // partial read or truncated; ignore 
            fprintf(stderr, "Server: partial read (%zd bytes)\n", r);
            continue;
        }

        // Print header info 
        printf("\n--- System Call Received ---\n");
        printf("Client pid: %d\n", req.pid);
        printf("System Call Requested: %d with %d parameter(s)\n",
               req.syscall_num, req.num_params);

        // parse parameters by splitting on '|' 
        char params_copy[MAX_PARAMS_LEN];
        params_copy[0] = '\0';
        if (req.param_size > 0 && req.param_size < MAX_PARAMS_LEN) {
            memcpy(params_copy, req.params, req.param_size);
            params_copy[req.param_size] = '\0';
        } else {
            params_copy[0] = '\0';
        }

        // Print parameters 
        if (req.num_params > 0 && strlen(params_copy) > 0) {
            char *saveptr = NULL;
            char *token = strtok_r(params_copy, "|", &saveptr);
            int p = 1;
            while (token && p <= req.num_params) {
                printf("Param%d=%s ", p, token);
                token = strtok_r(NULL, "|", &saveptr);
                ++p;
            }
            printf("\n");
        } else {
            printf("No parameters.\n");
        }

        // prepare default reply 
        rep.pid = req.pid;
        rep.result[0] = '\0';
        rep.result_size = 0;

        // Process system call 
        if (req.syscall_num == 1) { // CONNECT 
            // req.params contains client fifo name 
            char client_fifo_name[MAX_FIFO_NAME];
            strncpy(client_fifo_name, req.params, MAX_FIFO_NAME-1);
            client_fifo_name[MAX_FIFO_NAME-1] = '\0';
            printf("CONNECT request: client FIFO = %s\n", client_fifo_name);

            // open client fifo for writing (server will block until client opens read) 
            int client_fd = open(client_fifo_name, O_WRONLY);
            if (client_fd == -1) {
                perror("open(client_fifo) in CONNECT");
                // we still add client with fd = -1 so future requests can fail gracefully 
                add_client(req.pid, client_fifo_name, -1);
                snprintf(rep.result, sizeof(rep.result), "CONNECT_FAILED");
            } else {
                // add mapping 
                if (add_client(req.pid, client_fifo_name, client_fd) == -1) {
                    fprintf(stderr, "Server: client table full. Rejecting client %d\n", req.pid);
                    close(client_fd);
                    snprintf(rep.result, sizeof(rep.result), "CONNECT_REJECTED");
                } else {
                    snprintf(rep.result, sizeof(rep.result), "CONNECTED");
                    printf("Server: saved client %d -> %s (fd=%d)\n", req.pid, client_fifo_name, client_fd);
                }
            }
            rep.result_size = strlen(rep.result) + 1;

        } else if (req.syscall_num == 2) { // NUMBER: return the number 
            // assume param1 is integer string 
            char number_str[64] = {0};
            if (req.num_params >= 1) {
                strncpy(number_str, req.params, sizeof(number_str)-1);
            } else {
                strcpy(number_str, "0");
            }
            snprintf(rep.result, sizeof(rep.result), "%s", number_str);
            rep.result_size = strlen(rep.result) + 1;
            printf("Result=%s\n", rep.result);

        } else if (req.syscall_num == 3) { // TEXT: return the text 
            char text[MAX_PARAMS_LEN] = {0};
            if (req.num_params >= 1) {
                strncpy(text, req.params, sizeof(text)-1);
            }
            snprintf(rep.result, sizeof(rep.result), "%s", text);
            rep.result_size = strlen(rep.result) + 1;
            printf("Result=%s\n", rep.result);

        } else if (req.syscall_num == 4) { // STORE: store integer and return stored value 
            int val = 0;
            if (req.num_params >= 1) {
                val = atoi(req.params);
                stored_value = val;
            }
            snprintf(rep.result, sizeof(rep.result), "%d", stored_value);
            rep.result_size = strlen(rep.result) + 1;
            printf("Stored value = %d\n", stored_value);

        } else if (req.syscall_num == 5) { // RECALL: return stored value 
            snprintf(rep.result, sizeof(rep.result), "%d", stored_value);
            rep.result_size = strlen(rep.result) + 1;
            printf("Recalled value = %d\n", stored_value);

        } else if (req.syscall_num == 0) { // EXIT: client disconnects 
            printf("EXIT requested by client %d\n", req.pid);
            int idx = find_client_index(req.pid);
            if (idx != -1) {
                printf("Closing client FIFO (%s) fd=%d\n", clients[idx].fifo_name, clients[idx].fd);
                remove_client_index(idx);
            } else {
                printf("Client %d not found in map.\n", req.pid);
            }
            // No reply required for EXIT per instructions, and then continue 
            continue;

        } else if (req.syscall_num == -1) { // SHUTDOWN 
            printf("SHUTDOWN requested by client %d\n", req.pid);
            // inform all client s optionally 
            cleanup_all_clients();
            close(server_fd);
            unlink(SERVER_FIFO);
            printf("Server: shutdown complete. Exiting.\n");
            exit(EXIT_SUCCESS);

        } else {
            snprintf(rep.result, sizeof(rep.result), "UNKNOWN_SYSCALL");
            rep.result_size = strlen(rep.result) + 1;
            printf("Unknown syscall: %d\n", req.syscall_num);
        }

        // Send reply to this clients fifo (use mapping pid->fd) 
        int idx = find_client_index(req.pid);
        if (idx == -1) {
            fprintf(stderr, "Server: no client mapping for pid %d; cannot send reply\n", req.pid);
            continue;
        }
        int client_fd = clients[idx].fd;
        if (client_fd < 0) {
            fprintf(stderr, "Server: client fd for pid %d is invalid\n", req.pid);
            continue;
        }

        ssize_t w = write(client_fd, &rep, sizeof(Reply));
        if (w == -1) {
            perror("write(client_fifo)");
        } else {
            printf("Reply sent to client %d (fd=%d) Result=%s\n", req.pid, client_fd, rep.result);
        }
    }

    // cleanup on unexpected exit 
    cleanup_all_clients();
    close(server_fd);
    unlink(SERVER_FIFO);
    return 0;
}