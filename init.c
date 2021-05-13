#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/time.h>

int numberOfNodes;
int processPerNode;

void deleteq (int *queues){

    for (int i=0; i<numberOfNodes; i++){
        msgctl(queues[i], IPC_RMID, NULL);
    }

}

void nodo (int nodeId, int firstq, int numberOfNodes){
    char nodeId_str[20] = "";
    sprintf(nodeId_str, "%i", nodeId);

    char firstq_str[20] = "";
    sprintf(firstq_str, "%i", firstq); //queues[0]
    
    char numberOfNodes_str[20] = "";
    sprintf(numberOfNodes_str, "%i", numberOfNodes);
    
    char proccessPerNode_str[20] = "";
    sprintf(proccessPerNode_str, "%i",processPerNode);
    
    execl("./node", "./node", nodeId_str, firstq_str, numberOfNodes_str, proccessPerNode_str, NULL);

    exit(0);
}

int main (int argc, char* argv[]) {
    struct timeval stop_time, start_time;
    
    numberOfNodes = atoi(argv[1]);
    processPerNode = atoi(argv[2]);
    int queues[numberOfNodes];

    for (int i=0; i < numberOfNodes; i++){
	    queues[i] = msgget(IPC_PRIVATE, 0666 | IPC_CREAT); 
	
        if (i > 1) {
            if (queues[i-1] != queues[i]-1) {
                printf("\033[0;31m¡Las colas están desordenadas!\033[0m\n");
                exit(-1);
            }
        }

	    if (queues[i] == -1){
		    printf("No se ha podido crear el buzón.\n");
            return -1;
	    }
    }

    gettimeofday(&start_time, NULL);

    pid_t childs[numberOfNodes];

    for (int i=0; i < numberOfNodes; i++){

        pid_t child = fork();
        int nodeId = i*processPerNode;
        if (child == 0) nodo(nodeId, queues[0], numberOfNodes); // The child never return here

        childs[i] = child;
    }

    char option = 'n';
    while (option != 'q')
    {
        scanf("%c", &option);
    }

    for (size_t i = 0; i < numberOfNodes; i++)
    {
        kill(childs[i], SIGUSR1);
    }
    
    while (wait(NULL)!=-1);  //Esperamos a que todos los hijos mueran
    deleteq(queues);         //Borramos los buzones 

    gettimeofday(&stop_time, NULL);
    printf("\n[INIT] \033[0;33mExecuted %lu s\033[0m\n", (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_usec - start_time.tv_usec)/1000000); 
}