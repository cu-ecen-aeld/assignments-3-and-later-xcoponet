#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>

/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
struct thread_data{

    pthread_mutex_t *mutex;
    struct sockaddr_in client_addr;
    socklen_t client_sockfd;
    

    /**
     * Set to true if the thread completed with success, false
     * if an error occurred.
     */
    bool thread_complete_success;
};

typedef struct thread_data thread_data;
