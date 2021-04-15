#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h> // necesaria para ejecutar fork()
#include <string.h>
#include <signal.h>


void nodo ( int id, int firstq, int numNodos,int ProcesosNodo,int idNodo){
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

int main (int argc, char* argv[]){


    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int NumNodos= atoi(argv[3]);
    int ProcesosNodo = atoi(argv[4]);

    

    pid_t childs[ProcesosNodo];

    for (int i=0; i<ProcesosNodo; i++){

        pid_t child=fork();

        if (child==0) nodo(id+i,firstq,NumNodos,ProcesosNodo,id);      
        childs[i] = child;
        printf("Creado hijo %i  del  Nodo: %i \n",id+i,id);
    }

    char option = 'n';
    
    while (option != 'p')
    {
        scanf("%c", &option);
    }

    for (size_t i = 0; i < ProcesosNodo; i++)
    {
        kill(childs[i], SIGKILL);
    }
    
    while (wait(NULL)!=-1);


    
    return 0;
}