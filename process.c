#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include<math.h> //floor -> redondeo hacia bajo

sem_t mutex;

struct params{

	int *vecinos;
	int id;
    int idNodo;
    int ProcesosNodo;
    int numNodo;
}params ;

struct msgbuf{
	long mtype; // Tipo 1= request, Tipo 2= reply , Tipo 3= end.
	int nodo;
    int ticket;
    int dest;
};

int *pendientes;
int mi_ticket;
int max_ticket=0;
int quiero=0;
int n_pendientes=0;


void *receptor( void *params){

    struct msgbuf msg;
    struct params *data= (struct params *)params;

    int *vecinos= data->vecinos;
    int id= data->id;
    int idNodo= data->idNodo;
    int numNodo= data->numNodo;
    int ProcesosNodo= data->ProcesosNodo;
        
    while(1){
        msgrcv(vecinos[idNodo/ProcesosNodo], &msg, sizeof(int)*3, 1, 0);
        
        sem_wait(&mutex);

        if (msg.dest==id)
        {
            int queue= (int) floor (msg.nodo/ProcesosNodo);
            int dest=msg.nodo;
            printf("[Node %i]  Process: %i Received Request. Process: %i, Ticket: %i, Queue: %i \n", idNodo, id, msg.nodo, msg.ticket, vecinos[queue]);

            if (max_ticket<msg.ticket) max_ticket=msg.ticket;
            if (!quiero || msg.ticket<mi_ticket || (msg.ticket==mi_ticket && msg.nodo<id )) {
                printf("[Node %i] \033[0;32mAccepted Request: Process: %i, Ticket: %i, Queue: %i\033[0m\n\n", id, msg.nodo, msg.ticket, vecinos[queue]);
                msg.mtype=2;
                msg.dest=dest;
                msg.nodo=id;
                msg.ticket=0;
                msgsnd(vecinos[queue], &msg, sizeof(int)*3 , 0);
            }else {
                pendientes[n_pendientes++]=dest;
                printf("[Node %i] New waiting PROCESS %i\n\n", idNodo, dest);
             }

            
        }
        else {
            msgsnd(vecinos[idNodo/ProcesosNodo],&msg, sizeof(int)*3,0);
        }
        
        sem_post(&mutex);
        
    }

    pthread_exit(NULL);
}

int main (int argc, char* argv[]){

   
    sem_init(&mutex, 0, 1);
    
    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int numNodos= atoi(argv[3]);
    int ProcesosNodo = atoi(argv[4]);
    int idNodo= atoi(argv[5]);
   
    int vecinos[numNodos];
    pthread_t thread;
    pendientes=malloc (((numNodos*ProcesosNodo)-1) * sizeof(int)); 

    for (int j=0; j<numNodos;j++){
        vecinos[j]=firstq+j;
    }

    printf("[Node %i] \033[0;34mNode started. Process: %i. Queue: %i. Nodes: %i\033[0m\n", idNodo, id, vecinos[idNodo/ProcesosNodo], numNodos);
    
    params.id=id;
    params.vecinos=vecinos;
    params.idNodo=idNodo;
    params.numNodo=numNodos;
    params.ProcesosNodo=ProcesosNodo;

    if(pthread_create(&thread, NULL, (void *)receptor, (void *)&params)!=0) {
        printf("No se ha podido crear el hilo\n"); 
        exit(0);
    }

    // Pedir entrada ala seccion critica

    while (1)
    {   
        sem_wait(&mutex);
        quiero=1;
        mi_ticket=max_ticket+1;
        sem_post(&mutex);

        for (int i = 0; i < numNodos*ProcesosNodo; i++)
        {
            if(i!=id){
                struct msgbuf msg;
                int destQ=(int)floor(i/ProcesosNodo);
                msg.mtype=1;
                msg.nodo=id;
                msg.ticket=mi_ticket;
                msg.dest=(i);
                printf("[Node %i] \033[0;36mRequest sended: Process: %i, Ticket: %i, ToQueue: %i ToProcess: %i\033[0m \n", idNodo, msg.nodo, msg.ticket, vecinos[destQ], msg.dest);
                msgsnd(vecinos[destQ],&msg, sizeof(int)*3,0);


            }

        }
        
        printf("[Node %i]  Process: %i. Waiting...\n", idNodo, id);
        for (int i = 0; i < ((numNodos*ProcesosNodo)-1); i++)
        {
            struct msgbuf msg; 

            ssize_t x= msgrcv(vecinos[idNodo/ProcesosNodo], &msg, sizeof(int)*3, 2, 0);
            if (x==-1) printf("\033[0;31mError on msgrcv(). Queue: %i\033[0m\n", vecinos[idNodo/ProcesosNodo]);  
            if (msg.dest!=id){
                i--;
                msgsnd(vecinos[idNodo/ProcesosNodo],&msg, sizeof(int)*3,0);
            }
                     
        }

     
        //SECCION CRITICA
        printf("[Node %i]  Process: %i \033[0;35mDentro de la sección crítica.\033[0m Ticket: %i\n", idNodo, id,  mi_ticket);

        // Fuera de la sección crítica
        sem_wait(&mutex);
        quiero=0;
        sem_post(&mutex);

        printf("[Node %i]  Process: %i Fuera de la sección crítica\n\n", idNodo, id);

        for (int i = 0; i < n_pendientes; i++)
        {
            struct msgbuf msg;
            int nodoDest=(int)floor(pendientes[i]/ProcesosNodo);
            msg.mtype=2;
            msg.nodo=id;
            msg.ticket=0;
            msg.dest=pendientes[i];
            printf("[Node %i]  Process: %i \033[0;32mSending Reply Waiting  Process: %i, Queue: %i \033[0m\n\n", idNodo, id, pendientes[i], vecinos[nodoDest]);
            msgsnd(vecinos[nodoDest],&msg, sizeof(int)*3,0); 
        }
        n_pendientes=0;
    }
    
    return 0;
}