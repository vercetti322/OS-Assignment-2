#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "utils.h"
#include <string.h>
#include <sys/shm.h>

int main(void) 
{
    struct msg_buffer msg;
    key_t key;

    void *shared_memory;
    char buff[100];
    int shmid;

    // Connecting to the message queue created by the load balancer
    key = ftok("./loadBalancer.c", MSG_KEY);
    if (key == -1) 
    {
        perror("ftok");
        exit(-1);
    }

    int msgqid = msgget(key, 0666);
    shmid = shmget((key_t)1122, 1024, 0666 | IPC_CREAT);
    

    if (msgqid == -1) 
    {
        perror("msgget");
        exit(-2);
    }

    printf("Client: Connected to Message Queue with ID %d\n", msgqid);

    // Get client ID from the user
    long cid;
    printf("Enter client-id: ");
    scanf("%ld", &cid);

    // Clear the input buffer
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    //attaching shared memory to the process
    shared_memory = shmat(shmid, NULL, 0);
    printf("Process attached at %p\n", shared_memory);

    // Prepare a message to send
    msg.pyld.client_id = cid;
    msg.msg_type = 1;

    // Continuously interact with the load balancer
    while (1) {
        printf("Enter 1 for option 1\n");
        printf("Enter 2 for option 2\n");

        printf("Pick one: ");
        fgets(msg.pyld.msg_text, sizeof(msg.pyld.msg_text), stdin);

        int len = strlen(msg.pyld.msg_text);
        if (len > 0 && msg.pyld.msg_text[len - 1] == '\n') {
            // Removing the newline character if present
            msg.pyld.msg_text[len - 1] = '\0';
        }

        // Send the message to the load balancer
        if (strcmp(msg.pyld.msg_text, "1") == 0) 
        {
            msg.pyld.option = 1;
            strcpy(msg.pyld.msg_text, "Client: Hi, Server!");

            if (msgsnd(msgqid, &msg, sizeof(msg.pyld), 0) == -1) {
                perror("Error in msgsnd");
                exit(-3);
            }
        }

        // Client asking user to input and sending it to shared mem
        if (strcmp(msg.pyld.msg_text, "2") == 0) 
        {
            msg.pyld.option = 2;
            //asking users for input
            printf("What do you want to write in the shared segment?\n");
            read(0, buff, 100);
            //data written into shared memory
            strcpy(shared_memory, buff);
            printf("You wrote: %s\n", (char *)shared_memory);

            //send the load balancer the request type is 2
            if (msgsnd(msgqid, &msg, sizeof(msg.pyld), 0) == -1) {
                perror("Error in msgsnd");
                exit(-3);
            }
        }

        struct msg_buffer response;
        // Receive the response from the server with the correct message type
        if (msgrcv(msgqid, &response, sizeof(response.pyld), cid, 0) == -1) {
            perror("Error in msgrcv");
            exit(-6);
        }
        else {
            printf("Client: Received Response: %s\n", response.pyld.msg_text);
        }  
    }

    return 0;
}
