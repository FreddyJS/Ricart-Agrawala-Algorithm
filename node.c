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

typedef struct child {
    pid_t pid;
    int type;
} child_t;

int end = 0;

void process ( int id, int type, int firstq, int numberOfNodes,int processPerNode,int idNodo, int sems_key, int data_key){
    char id_str[20] = "";
    sprintf(id_str, "%i", id);
    char type_str[20] = "";
    sprintf(type_str, "%i", type);
    char firstq_str[20] = "";
    sprintf(firstq_str, "%i", firstq); //queues[0]
    char numOfNodes_str[20] = "";
    sprintf(numOfNodes_str, "%i", numberOfNodes);
    char processPerNode_str[20] = "";
    sprintf(processPerNode_str, "%i",processPerNode);
    char idNodo_str[20] = "";
    sprintf(idNodo_str, "%i",idNodo);
    char sems_key_str[20] = "";
    sprintf(sems_key_str, "%i",sems_key);
    char data_key_str[20] = "";
    sprintf(data_key_str, "%i",data_key);

    execl("./process", "./process", id_str, type_str, firstq_str, numOfNodes_str, processPerNode_str, idNodo_str, sems_key_str, data_key_str, NULL);

    exit (0);
}

void end_handler(int signo) {
    end = 1;
}

void autoAcceptTicket(int dest, int queue, int firstq, int nodeId, int maxpos, int pos, ticket_t* tickets_mem, sem_t* sems_mem) {
    ticketok_t msg;
    msg.mtype = TICKETOK;
    msg.dest = dest;
    //msg.org_process = -1;
    //msg.org_node = -1;

    /*if (queue == firstq + nodeId) {
        sem_wait(&sems_mem[maxpos - pos]);  // wait for the child to be ready to read data
        memcpy(&tickets_mem[pos], &msg, sizeof(ticketok_t)); // Copy the data so the child can read it
        sem_post(&sems_mem[pos]);  // Post to the child

        return;
    }*/
            
    msgsnd(queue, &msg, sizeof(int), 0); 
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

    child_t childs[processPerNode];

    for (int i=0; i<processPerNode; i++){
        int type = EVENTOS;
        if (i > processPerNode/4) type = GRADAS;
        if (i > 2*processPerNode/4) type = PRERESERVAS;
        if (i > 3*processPerNode/4) type = ADMIN;

        pid_t child=fork();
        int processID = i;

        if (child==0) process(processID, type, firstq,numberOfNodes,processPerNode,nodeId,sems_key,data_key);      
        childs[i].pid = child;
        childs[i].type = type;

    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = end_handler;

    sigaction(SIGUSR1, &sigact, NULL); // Create a handler so the program not exit when SIGUSR1 arrives

    printf("\033[0;34m[Node %i] Ready to listen for messages!\033[0m\n", nodeId);

    ticket_t request;
    ticketok_t response;

    int myqueue = firstq + nodeId;
    int maxpos = processPerNode*2 -1; // max position of the array of sems
    
    while (!end)
    {
        // Receive a message from the queue, th size is the max size of the message
        // So we can set it to the bigger type
        msgrcv(myqueue, &request, sizeof(int)*4, 0, 0);

        if (request.mtype == TICKETOK) {
            // Copy the data to the ticketok_t
            memcpy(&response, &request, sizeof(ticketok_t));
            int pos = response.dest; // Position of the array in the shared memory

            sem_wait(&sems_mem[maxpos - pos]);  // wait for the child to be ready to read data
            memcpy(&tickets_mem[pos], &response, sizeof(ticketok_t)); // Copy the data so the child can read it
            sem_post(&sems_mem[pos]);  // Post to the child

        } else {
            //printf("[Node %i] \033[0;33mRecibida peticion de: node %i, process %i\033[0m\n", nodeId, request.node, request.process);
            // Avisar a todos los procesos del nodo
            for (int i = 0; i < processPerNode; i++)
            {
                if (request.node == nodeId && i == request.process) continue; // Skip the process who send the request

                // Los lectores son automáticamente aceptados en lugar de preguntar a los demás lectores
                if ((childs[i].type == EVENTOS || childs[i].type == GRADAS) && (request.type == EVENTOS || request.type == GRADAS)){
                    autoAcceptTicket(request.process, firstq + request.node, firstq, nodeId, maxpos, i, tickets_mem, sems_mem);
                    continue;
                } 
                                
                sem_wait(&sems_mem[maxpos - i]);
                memcpy(&tickets_mem[i], &request, sizeof(ticket_t)); // Copy the data so the child can read it
                sem_post(&sems_mem[i]);
            }
        }

    }

    for (size_t i = 0; i < processPerNode; i++)
    {
        kill(childs[i].pid, SIGUSR1);
    }
    
    while (wait(NULL) != -1);

    free(sems_mem);
    free(tickets_mem);

    return 0;
}