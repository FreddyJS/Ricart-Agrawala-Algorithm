#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h> // necesaria para ejecutar fork()
#include <string.h>
#include <math.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/shm.h> 

#include "tickets.h"

int end = 0;

void process ( int id, int firstq, int numNodos,int ProcesosNodo,int idNodo, int sems_key, int data_key){
    char param1[20] = "";
    sprintf(param1, "%i", id);
    char param2[20] = "";
    sprintf(param2, "%i", firstq); //ID[0]
    char param3[20] = "";
    sprintf(param3, "%i", numNodos);
    char param4[20] = "";
    sprintf(param4, "%i",ProcesosNodo);
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
    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int NumNodos= atoi(argv[3]);
    int ProcesosNodo = atoi(argv[4]);

    int sems_key = shmget(IPC_PRIVATE, sizeof(sem_t)*ProcesosNodo*2, IPC_CREAT | IPC_EXCL | 0666);
    int data_key = shmget(IPC_PRIVATE, sizeof(ticket_t)*ProcesosNodo, IPC_CREAT | IPC_EXCL | 0666);

    ticket_t* tickets_mem = (ticket_t *)shmat(data_key, NULL, 0);
    sem_t* sems_mem = (sem_t *)shmat(sems_key, NULL, 0);


    for (int i = 0; i < ProcesosNodo; i++)
    {
        sem_init(&sems_mem[i], 1, 0);
    }
    
    for (int i = ProcesosNodo; i < ProcesosNodo*2; i++)
    {
        printf("[Node %i] init sem %i\n", id, i);
        sem_init(&sems_mem[i], 1, 1);
    }

    // Tantos semaforos como procesos por nodo
    // 50 bytes para array de chars // 

    pid_t childs[ProcesosNodo];

    for (int i=0; i<ProcesosNodo; i++){

        pid_t child=fork();

        if (child==0) process(id+i,firstq,NumNodos,ProcesosNodo,id,sems_key,data_key);      
        childs[i] = child;
        printf("[Node %i]  Created process: %i\n",id, id+i);
    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = end_handler;

    sigaction(SIGUSR1, &sigact, NULL); // Create a handler so the program not exit when SIGUSR1 arrives

    ticket_t msg;
    int myqueue = firstq + (int)floor(id/ProcesosNodo);
    
    while (!end)
    {
        msgrcv(myqueue, &msg, sizeof(int)*3, 0, 0);
        int process = msg.dest;
        int pos = process - id;
        int maxpos = ProcesosNodo*2 -1;
        //printf("[Node %i] \033[0;33m Semaforos: wait(%i); post(%i); \033[0m\n", id, maxpos -pos, pos);
        
        // Si puedo escribir (zona de memoria libre o ya usada) 5 procesos --> [0, 4] [5, 9]
        sem_wait(&sems_mem[maxpos - pos]); 

        tickets_mem[pos].dest = process;
        tickets_mem[pos].mtype = msg.mtype;
        tickets_mem[pos].ticket = msg.ticket;
        tickets_mem[pos].nodo = msg.nodo;

        // Avisar a process
        sem_post(&sems_mem[pos]);
    }

    for (size_t i = 0; i < ProcesosNodo; i++)
    {
        kill(childs[i], SIGKILL);
    }
    
    while (wait(NULL)!=-1);

    return 0;
}