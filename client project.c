//Client code for Programming Assignment 1
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
#define MAX_FIFO_NAME 128
// Params = Parameters, didnt wanna keep typing parameters
typedef struct {
    pid_t pid;
    int syscall_num;
    int num_params;
    int param_size;
    char params[MAX_PARAMS_LEN];
} Request;

typedef struct {
    pid_t pid;
    int result_size;
    char result[512];
} Reply;

static void strip_newline(char *s) {
    size_t l = strlen(s);
    if (l > 0 && s[l-1] == '\n') s[l-1] = '\0';
}

int main(void) {
    pid_t pid = getpid();
    char client_fifo[MAX_FIFO_NAME];
    snprintf(client_fifo, sizeof(client_fifo), "./client_%d_fifo", pid);

    // Create client FIFO 
    if (mkfifo(client_fifo, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo(client fifo)");
        exit(EXIT_FAILURE);
    }

    // Open server FIFO for writing 
    int server_fd = open(SERVER_FIFO, O_WRONLY);
    if (server_fd == -1) {
        perror("open(server_fifo)");
        unlink(client_fifo);
        exit(EXIT_FAILURE);
    }

    // Prepare and send CONNECT request 
    Request req;
    memset(&req, 0, sizeof(req));
    req.pid = pid;
    req.syscall_num = 1;
    req.num_params = 1;
    strncpy(req.params, client_fifo, sizeof(req.params)-1);
    req.param_size = strlen(req.params) + 1;

    if (write(server_fd, &req, sizeof(req)) == -1) {
        perror("write CONNECT");
        close(server_fd);
        unlink(client_fifo);
        exit(EXIT_FAILURE);
    }

    // Open client FIFO for reading (this will block until server opens write) 
    int client_fd = open(client_fifo, O_RDONLY);
    if (client_fd == -1) {
        perror("open(client_fifo) for read");
        close(server_fd);
        unlink(client_fifo);
        exit(EXIT_FAILURE);
    }

    printf("Client (pid=%d) connected. Client FIFO: %s\n", pid, client_fifo);

    while (1) {
        printf("\nChoose action:\n");
        printf("1 - Send request\n");
        printf("2 - EXIT (close this client)\n");
        printf("3 - SHUTDOWN (ask server to exit)\n");
        printf("Enter choice: ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            // Clear input 
            int c; while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }
        // Clear newline 
        int ch = getchar();

        if (choice == 1) {
            printf("Enter system call number (2=Number, 3=Text, 4=Store, 5=Recall): ");
            int sc = 0;
            if (scanf("%d", &sc) != 1) {
                int c; while ((c = getchar()) != '\n' && c != EOF) {}
                continue;
            }
            ch = getchar();

            req.pid = pid;
            req.syscall_num = sc;

            if (sc == 5) {
                // Recall: no parameters 
                req.num_params = 0;
                req.param_size = 0;
                req.params[0] = '\0';
            } else {
                printf("How many parameters (enter 0..10): ");
                int n = 0;
                if (scanf("%d", &n) != 1) { int c; while ((c = getchar()) != '\n' && c != EOF) {} continue;}
                if (n < 0) n = 0;
                if (n > 10) n = 10;
                ch = getchar();

                req.num_params = n;
                // Gather params 
                char buffer[MAX_PARAMS_LEN];
                buffer[0] = '\0';
                for (int i = 0; i < n; ++i) {
                    char tmp[128];
                    printf("Enter parameter %d: ", i+1);
                    if (!fgets(tmp, sizeof(tmp), stdin)) tmp[0] = '\0';
                    strip_newline(tmp);
                    // append with '|' separator if not first 
                    if (i > 0) strncat(buffer, "|", sizeof(buffer)-strlen(buffer)-1);
                    strncat(buffer, tmp, sizeof(buffer)-strlen(buffer)-1);
                }
                // Copy to req.params 
                strncpy(req.params, buffer, sizeof(req.params)-1);
                req.param_size = strlen(req.params) + 1;
            }

            // Write request to server 
            if (write(server_fd, &req, sizeof(req)) == -1) {
                perror("write to server_fifo");
                break;
            }

            // Read reply from client FIFO (blocking read) 
            Reply rep;
            ssize_t r = read(client_fd, &rep, sizeof(rep));
            if (r > 0) {
                printf("Reply from server (pid=%d): %s\n", rep.pid, rep.result);
            } else if (r == 0) {
                printf("Server closed client FIFO or sent no data.\n");
            } else {
                perror("read reply");
            }

        } else if (choice == 2) { // EXIT 
            Request exit_req;
            exit_req.pid = pid;
            exit_req.syscall_num = 0;
            exit_req.num_params = 0;
            exit_req.param_size = 0;
            exit_req.params[0] = '\0';
            if (write(server_fd, &exit_req, sizeof(exit_req)) == -1) perror("write EXIT");
            // Close and cleanup 
            close(client_fd);
            unlink(client_fifo);
            close(server_fd);
            printf("Client exiting.\n");
            break;

        } else if (choice == 3) { // SHUTDOWN 
            Request sd;
            sd.pid = pid;
            sd.syscall_num = -1;
            sd.num_params = 0;
            sd.param_size = 0;
            sd.params[0] = '\0';
            if (write(server_fd, &sd, sizeof(sd)) == -1) perror("write SHUTDOWN");
            close(client_fd);
            unlink(client_fifo);
            close(server_fd);
            printf("Client requested server shutdown and exits.\n");
            break;

        } else {
            printf("Invalid choice.\n");
        }
    }

    return 0;
}