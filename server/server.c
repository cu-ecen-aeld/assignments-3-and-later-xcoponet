
#include <stdio.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

bool aborted = false;

static void sig_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        printf("Received SIGINT or SIGTERM\n");
        syslog(LOG_DEBUG, "Caught signal, exiting\n");
        aborted = true;
        remove("/var/tmp/aesdsocketdata");
    }    
}

int main()
{
    printf("Hello, World!\n");
    // Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. 
    // Setup syslog logging
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    //setup sigaction
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1) || (sigaction(SIGTERM, &act, NULL) == -1))
    {
        perror("Failed to set signal handler");
        return 1;
    }
       

    // Open a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        return -1;
    }
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(sockfd, 10) == -1) {
        perror("listen");
        return -1;
    }

    while(!aborted)
    {
        // Listens for and accepts a connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sockfd == -1) {
            if (errno == EINTR) {
                // Interrupted by signal
                continue;
            }

            perror("accept");
            return -1;
        }


        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        // Log the message
        syslog(LOG_DEBUG, "Accepted connection from %s\n", client_ip);
        printf("Accepted connection from %s\n", client_ip);


        // Receive data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
        FILE *file = fopen("/var/tmp/aesdsocketdata", "a+");
        if (file == NULL) {
            perror("fopen");
            return -1;
        }
        char buffer[1024];
        ssize_t bytes_received;
        while ((bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0)) > 0) {
            fwrite(buffer, sizeof(char), bytes_received, file);
            fwrite(buffer, sizeof(char), bytes_received, stdout);
            // if line break is received, break the loop
            if (buffer[bytes_received - 1] == '\n') {
                break;
            }
        }
        if (bytes_received == -1) {
            perror("recv");
            return -1;
        }
        fclose(file);

        // Return the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
        file = fopen("/var/tmp/aesdsocketdata", "r");
        if (file == NULL) {
            perror("fopen");
            return -1;
        }
        while ((bytes_received = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
            printf("Sending %ld bytes\n", bytes_received);
            fwrite(buffer, sizeof(char), bytes_received, stdout);
            printf("\n");
            if(send(client_sockfd, buffer, bytes_received, 0) == -1) {
                perror("send");
                return -1;
            }
        }
        if (bytes_received == -1) {
            perror("fread");
            return -1;
        }
        fclose(file);

        // sleep(5);

        // Close the connection
        shutdown(client_sockfd, 2);
        // Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
        // Log the message
        syslog(LOG_DEBUG, "Closed connection from %s\n", client_ip);
        printf("Closed connection from %s\n", client_ip);
    }
    

    // Clean up syslog
    closelog();

    printf("Exit");
    return 0;
}
