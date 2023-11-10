#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "utils.h"
#include <string.h>
#include <sys/shm.h>

int msgqid;

// Signal handler to exit gracefully on Ctrl+C (SIGINT)
void handle_sigint(int signo) {
    printf("\nLoad Balancer: Received Ctrl+C. Cleaning up and exiting...\n");
    
    // Remove the message queue before exiting
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
    }

    exit(0);
}

int main(void) {
    struct msg_buffer msg;
    key_t key;

    void *shared_memory;
    char bufff[100];
    int shmid;

    // Creating a message queue
    key = ftok("./loadBalancer.c", MSG_KEY);
    if (key == -1) {
        perror("ftok");
        exit(-1);
    }

    msgqid = msgget(key, 0666 | IPC_CREAT);
    if (msgqid == -1) {
        perror("msgget");
        exit(-2);
    }
    printf("Load Balancer: Message Queue Created with ID %d\n", msgqid);

    // Set up a signal handler to remove the message queue on Ctrl+C (SIGINT)
    signal(SIGINT, handle_sigint);

    // Continuously listen for messages
    while (1) {
        
        // Receive a message
        if (msgrcv(msgqid, &msg, sizeof(msg.pyld), 1, 0) == -1) {
            perror("msgrcv");
            exit(-3);
        } 
        else {

            struct msg_buffer response_msg;// defining response to be sent to client
            response_msg.msg_type = msg.pyld.client_id;
            response_msg.pyld.client_id=msg.pyld.client_id;
            response_msg.pyld.option=msg.pyld.option;

            if(msg.pyld.option == 1) 
            {
                printf("\n I got 1 from client id: %ld\n", msg.pyld.client_id);
                printf("%s\n", msg.pyld.msg_text);
                strcpy(response_msg.pyld.msg_text, "I got 1");
            }
            
            if(msg.pyld.option == 2)
            {
                printf("\n I got 2 from client id: %ld\n", msg.pyld.client_id); 
                strcpy(response_msg.pyld.msg_text, "I got 2");
                //reading from the shared segment
                shmid = shmget((key_t)1122, 1024, 0666);
                //attach loadBalancer to the shared memory
                shared_memory = shmat(shmid, NULL, 0);
                //print the data read from the shared segment
                printf("Data read from shared memory id: %s\n", shared_memory);
            }

            if (msgsnd(msgqid, &response_msg, sizeof(response_msg.pyld), 0) == -1)
            {
                    perror("msgsnd");
                    exit(-1);
            }
        }
    }
    return 0;
}
