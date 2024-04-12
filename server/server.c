
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <syslog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "queue.h"
#include "threading.h"

bool aborted = false;


// The data type for the node
struct ThreadListNode
{
    pthread_t thread;
    thread_data *thread_data;
    // This macro does the magic to point to other nodes
    TAILQ_ENTRY(ThreadListNode) nodes;
};


static void sig_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        printf("Received SIGINT or SIGTERM (%d)\n", signo);
        syslog(LOG_DEBUG, "Caught signal %d, exiting\n", signo);
        aborted = true;
        remove("/var/tmp/aesdsocketdata");
    }
}

static void timer_thread(union sigval sigval)
{
    pthread_mutex_t *mutex = (pthread_mutex_t *) sigval.sival_ptr;
    printf("Timer thread\n");
    syslog(LOG_DEBUG, "Timer thread\n");

    if(pthread_mutex_lock(mutex) != 0)
    {
        perror("pthread_mutex_lock");
        return;
    }
    else
    {
        FILE *file = fopen("/var/tmp/aesdsocketdata", "a");
        if (file == NULL) {
            perror("fopen");
            // pthread_mutex_unlock(mutex);
            return;
        }
        time_t t = time(NULL);
        char timestr[100];
        strftime(timestr, 100, "%a, %d %b %Y %T %z", localtime(&t));
        fprintf(file, "timestamp:%s\n", timestr);
        printf("timestamp:%s\n", timestr);
        fclose(file);
        pthread_mutex_unlock(mutex);
    }

}



void demonize()
{
    printf("Daemonizing\n");
    syslog(LOG_DEBUG, "Daemonizing\n");
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork");
        exit( EXIT_FAILURE );
    }
    if(pid > 0)
    {
        // parent process
        exit(0);
    }
    // child process
    if(setsid() < 0)
    {
        perror("setsid");
        exit( EXIT_FAILURE );
    }
    if ( chdir("/") != 0 ) {
        fprintf( stderr, "Impossible de se placer dans le dossier %s.\n", "/" );
        exit( EXIT_FAILURE );
    }   


    //redirect stdout, sdterr and stdin to /dev/null
    // Open /dev/null for reading and writing
    int devNull = open("/dev/null", O_RDWR);

    if (devNull == -1) {
        perror("Failed to open /dev/null");
        exit( EXIT_FAILURE );
    }

    // Redirect stdin to /dev/null
    if (dup2(devNull, STDIN_FILENO) == -1) {
        perror("Failed to redirect stdin to /dev/null");
        exit( EXIT_FAILURE );
    }

    // Redirect stdout to /dev/null
    if (dup2(devNull, STDOUT_FILENO) == -1) {
        perror("Failed to redirect stdout to /dev/null");
        exit( EXIT_FAILURE );
    }

    // Redirect stderr to /dev/null
    if (dup2(devNull, STDERR_FILENO) == -1) {
        perror("Failed to redirect stderr to /dev/null");
        exit( EXIT_FAILURE );
    }

    // After redirecting, the devNull file descriptor is no longer needed
    // It can be closed to free up resources
    if (close(devNull) == -1) {
        perror("Failed to close file descriptor for /dev/null");
        exit( EXIT_FAILURE );
    }
}

void* handle_client(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct sockaddr_in client_addr = thread_func_args->client_addr;
    socklen_t client_sockfd = thread_func_args->client_sockfd;


    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    // Log the message
    syslog(LOG_DEBUG, "Accepted connection from %s\n", client_ip);
    printf("Accepted connection from %s\n", client_ip);

    pthread_mutex_t *mutex = thread_func_args->mutex;
    pthread_mutex_lock(mutex);

    // Receive data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
    FILE *file = fopen("/var/tmp/aesdsocketdata", "a+");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(mutex);
        pthread_exit(NULL);
    }
    char buffer[1024];
    ssize_t bytes_received;
    while ((bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, sizeof(char), bytes_received, file);
        printf("Received %ld bytes\n", bytes_received);
        fwrite(buffer, sizeof(char), bytes_received, stdout);
        printf("\n");
        char * str = (char*) malloc(bytes_received + 1);
        memcpy(str, buffer, bytes_received);
        str[bytes_received] = '\0';
        syslog(LOG_DEBUG, "%s", str);
        free(str);
        // if line break is received, break the loop
        if (buffer[bytes_received - 1] == '\n') {
            break;
        }
    }
    if (bytes_received == -1) {
        perror("recv");
    }
    fclose(file);
    pthread_mutex_unlock(mutex);

    // Return the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
    file = fopen("/var/tmp/aesdsocketdata", "r");
    if (file == NULL) {
        perror("fopen");
        pthread_exit(NULL);
    }
    while ((bytes_received = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0) {
        printf("Sending %ld bytes\n", bytes_received);
        // fwrite(buffer, sizeof(char), bytes_received, stdout);
        printf("\n");
        char * str = (char*) malloc(bytes_received + 1);
        memcpy(str, buffer, bytes_received);
        str[bytes_received] = '\0';
        syslog(LOG_DEBUG, "%s\n", str);
        free(str);
        if(send(client_sockfd, buffer, bytes_received, 0) == -1) {
            perror("send");
            break;
        }
    }
    if (bytes_received == -1) {
        perror("fread");
    }
    fclose(file);

    // Close the connection
    shutdown(client_sockfd, 2);
    // Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
    // Log the message
    syslog(LOG_DEBUG, "Closed connection from %s\n", client_ip);
    printf("Closed connection from %s\n", client_ip);

    thread_func_args->thread_complete_success = true;
    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
    printf("Hello, World!\n");
    // Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. 
    // Setup syslog logging
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // remove /var/tmp/aesdsocketdata if exists
    remove("/var/tmp/aesdsocketdata");

    // create a mutex for the log file
    pthread_mutex_t mutex;
    if(pthread_mutex_init(&mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        return 1; // or appropriate error handling
    }

    //setup sigaction
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sig_handler;
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)|| (sigaction(SIGTERM, &act, NULL) == -1))
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

    // add argument -d
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        demonize();
    }

    struct  sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_thread;
    sev.sigev_value.sival_ptr = &mutex;
    timer_t timer;
    if(timer_create(CLOCK_MONOTONIC, &sev, &timer) != 0)
    {
        perror("timer_create");
        return 1;
    }
    else
    {
        printf("timer created\n");
        struct itimerspec timerSpec;
        timerSpec.it_value.tv_sec = 10;
        timerSpec.it_value.tv_nsec = 0;
        timerSpec.it_interval.tv_sec = 10;  // Interval for periodicity
        timerSpec.it_interval.tv_nsec = 0;
        // Start the timer
        if(timer_settime(timer, 0, &timerSpec, NULL))
        {
            perror("timer_settime");
            return 1;
        }
    }

    // declare the head
    TAILQ_HEAD(head_s, ThreadListNode) head;
    // Initialize the head before use
    TAILQ_INIT(&head);

    while(!aborted)
    {
        struct ThreadListNode * threadlistnode;

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

        // create a new threadlistnode
        threadlistnode = malloc(sizeof(struct ThreadListNode));
        if (threadlistnode == NULL)
        {
            fprintf(stderr, "malloc failed");
            syslog(LOG_ERR, "malloc failed");
            break;
        }

        // create a new thread to handle the client
        struct thread_data * thread_data = malloc(sizeof(struct thread_data));
        if (thread_data == NULL)
        {
            fprintf(stderr, "malloc failed");
            syslog(LOG_ERR, "malloc failed");
            free(threadlistnode);
            break;
        }
        thread_data->mutex = &mutex;
        thread_data->client_addr = client_addr;
        thread_data->client_sockfd = client_sockfd;
        thread_data->thread_complete_success = false;
        threadlistnode->thread_data = thread_data;

        if(pthread_create(&(threadlistnode->thread), NULL, (void *(*)(void *))handle_client, (void *) thread_data) != 0)
        {
            fprintf(stderr, "pthread_create failed");
            syslog(LOG_ERR, "pthread_create failed");
            free(threadlistnode->thread_data);
            free(threadlistnode);
            break;
        }

        //add the thread to the list
        TAILQ_INSERT_TAIL(&head, threadlistnode, nodes);
        threadlistnode = NULL;
        
    }

    printf("Cleaning up\n");
    // Clean up the threads
    struct ThreadListNode * threadlistnode = NULL;
    while (!TAILQ_EMPTY(&head))
    {
        threadlistnode = TAILQ_FIRST(&head);
        pthread_join(threadlistnode->thread, NULL);
        TAILQ_REMOVE(&head, threadlistnode, nodes);
        free(threadlistnode->thread_data);
        free(threadlistnode);
        threadlistnode = NULL;
    }
    
    timer_delete(timer);

    // Clean up syslog
    closelog();

    printf("Exit");
    return 0;
}


