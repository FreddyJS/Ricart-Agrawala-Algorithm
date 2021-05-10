#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h> // necesaria para ejecutar fork()
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/shm.h> 

#include "tickets.h"

int end = 0;

void process ( int id, int firstq, int numberOfNodes,int processPerNode,int idNodo, int sems_key, int data_key){
    char param1[20] = "";
    sprintf(param1, "%i", id);
    char param2[20] = "";
    sprintf(param2, "%i", firstq); //queues[0]
    char param3[20] = "";
    sprintf(param3, "%i", numberOfNodes);
    char param4[20] = "";
    sprintf(param4, "%i",processPerNode);
    char param5[20] = "";
    sprintf(param5, "%i",idNodo);
    char param6[20] = "";
    sprintf(param6, "%i",sems_key);
    char param7[20] = "";
    sprintf(param7, "%i",data_key);

    execl("./process", "./process", param1, param2, param3, param4, param5, param6, param7, NULL);

    exit (0);
}

void end_handler(int signo) {
    end = 1;
}

int main (int argc, char* argv[]){
    int nodeId = atoi(argv[1]);
    int firstq = atoi(argv[2]);
    int numberOfNodes = atoi(argv[3]);
    int processPerNode = atoi(argv[4]);

    // Half of the sem are for the child to read after the node sets the values on mem (upperhalf)
    // The lowerhalf is to wait the child to read the data before we overwrite it
    int sems_key = shmget(IPC_PRIVATE, sizeof(sem_t)*processPerNode*2, IPC_CREAT | IPC_EXCL | 0666);
    int data_key = shmget(IPC_PRIVATE, sizeof(ticket_t)*processPerNode, IPC_CREAT | IPC_EXCL | 0666);

    ticket_t* tickets_mem = (ticket_t *)shmat(data_key, NULL, 0);
    sem_t* sems_mem = (sem_t *)shmat(sems_key, NULL, 0);

    for (int i = 0; i < processPerNode; i++)
    {
        sem_init(&sems_mem[i], 1, 0);
    }
    
    for (int i = processPerNode; i < processPerNode*2; i++)
    {
        sem_init(&sems_mem[i], 1, 1);
    }

    pid_t childs[processPerNode];

    for (int i=0; i<processPerNode; i++){

        pid_t child=fork();

        if (child==0) process(nodeId+i,firstq,numberOfNodes,processPerNode,nodeId,sems_key,data_key);      
        childs[i] = child;
        //printf("[Node %i]  Created process: %i\n",id, id+i);
    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = end_handler;

    sigaction(SIGUSR1, &sigact, NULL); // Create a handler so the program not exit when SIGUSR1 arrives

    printf("\033[0;34m[Node %i] Ready to listen for messages!\033[0m\n", nodeId/numberOfNodes);

    ticket_t request;
    ticketok_t response;

    int myqueue = firstq + nodeId/processPerNode;
    int maxpos = processPerNode*2 -1; // max position of the array of sems
    
    while (!end)
    {
        // Receive a message from the queue, th size is the max size of the message
        // So we can set it to the bigger type
        msgrcv(myqueue, &request, sizeof(int)*2, 0, 0);

        if (request.mtype == 2) {
            // Copy the data to the ticketok_t
            memcpy(&response, &request, sizeof(ticketok_t));
            int pos = response.dest - nodeId; // Position of the array in the shared memory

            sem_wait(&sems_mem[maxpos - pos]);  // wait for the child to be ready to read data
            memcpy(&tickets_mem[pos], &response, sizeof(ticketok_t)); // Copy the data so the child can read it
            sem_post(&sems_mem[pos]);  // Post to the child

        } else {
            // Avisar a todos los procesos del nodo
            for (size_t i = nodeId; i < nodeId+processPerNode; i++)
            {
                if (i == request.process) continue;
                int pos = i - nodeId;
                
                //printf("[Node %i] \033[0;33m Semaforos: wait(%i); post(%i); \033[0m\n", id, maxpos -pos, pos);
                
                sem_wait(&sems_mem[maxpos - pos]);
                memcpy(&tickets_mem[pos], &request, sizeof(ticket_t)); // Copy the data so the child can read it
                sem_post(&sems_mem[pos]);
            }
        }

    }

    for (size_t i = 0; i < processPerNode; i++)
    {
        kill(childs[i], SIGKILL);
    }
    
    while (wait(NULL)!=-1);

    return 0;
}