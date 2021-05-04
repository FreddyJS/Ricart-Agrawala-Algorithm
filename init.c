#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h> // necesaria para ejecutar fork()
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/time.h>

int NumNodos;
int ProcesosNodo;

void deleteq (int *ID){

    for (int i=0; i<NumNodos; i++){
        msgctl(ID[i], IPC_RMID, NULL);
    }

}

void nodo (int id, int firstq, int numNodos){
    char param1[20] = "";
    sprintf(param1, "%i", id);
    char param2[20] = "";
    sprintf(param2, "%i", firstq); //ID[0]
    char param3[20] = "";
    sprintf(param3, "%i", numNodos);
    char param4[20] = "";
    sprintf(param4, "%i",ProcesosNodo);
    
    execl("./node", "./node", param1, param2, param3, param4, NULL);

    exit (0);
}

int main (int argc, char* argv[]) {
    struct timeval stop_time, start_time;
    NumNodos= atoi(argv[1]);
    ProcesosNodo=atoi(argv[2]);
    int ID [NumNodos];

    for (int i=0; i<NumNodos; i++){
	
	    ID[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT); 
	
	    if (ID[i] == -1){
		    printf("No se ha podido crear el buzón.\n");
		
            return -1;
	    }
    }

    gettimeofday(&start_time, NULL);
    pid_t childs[NumNodos];

    for (int i=0; i<NumNodos; i++){

        pid_t child=fork();

        if (child==0) nodo(i*ProcesosNodo,ID[0],NumNodos);   //NumNodos ya es global?¿     
        childs[i] = child;
    }

    char option = 'n';
    
    while (option != 'q')
    {
        scanf("%c", &option);
    }

    for (size_t i = 0; i < NumNodos; i++)
    {
        kill(childs[i], SIGUSR1);
    }
    
    while (wait(NULL)!=-1);  //Esperamos a que todos los hijos mueran
    deleteq(ID);    //Borramos los buzones 

    gettimeofday(&stop_time, NULL);

    printf("\n[INIT] \033[0;33mExecuted %lu s\033[0m\n", (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_usec - start_time.tv_usec)/1000000); 
}