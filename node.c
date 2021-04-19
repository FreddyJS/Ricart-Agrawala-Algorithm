#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h> // necesaria para ejecutar fork()
#include <string.h>
#include <math.h>
#include <signal.h>

#include "tickets.h"

int end = 0;

void process ( int id, int firstq, int numNodos,int ProcesosNodo,int idNodo){
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
    
    execl("./process", "./process", param1, param2, param3, param4, param5, NULL);

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

    pid_t childs[ProcesosNodo];

    for (int i=0; i<ProcesosNodo; i++){

        pid_t child=fork();

        if (child==0) process(id+i,firstq,NumNodos,ProcesosNodo,id);      
        childs[i] = child;
        printf("[Node %i]  Created process: %i\n",id, id+i);
    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = end_handler;

    sigaction(SIGUSR1, &sigact, NULL); // Create a handler so the program not exit when SIGUSR1 arrives

    pause(); // After receive SIGUSR1 from the init process we can delete all the childs

    /*
    ticket_t msg;
    int myqueue = firstq + (int)floor(id/ProcesosNodo);
    
    sleep(1);
    while (!end)
    {
        msgrcv(myqueue, &msg, sizeof(int)*3, 0, 0);
        int process = msg.dest;
        msgsnd(myqueue, &msg, sizeof(int)*3, 0);
        kill(childs[process - id], SIGCONT);
        pause();
    }
    */

    for (size_t i = 0; i < ProcesosNodo; i++)
    {
        kill(childs[i], SIGKILL);
    }
    
    while (wait(NULL)!=-1);

    return 0;
}