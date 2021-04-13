#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>

sem_t mutex;

struct params{

	int *vecinos;
	int id;

}params ;

struct msgbuf{
	long mtype; // Tipo 1= request, Tipo 2= reply , Tipo 3= end.
	int nodo;
    int ticket;
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
        
    while(1){

        if (msgrcv(vecinos[id], &msg, sizeof(int)*2, 3, IPC_NOWAIT)!=-1) break;
        msgrcv(vecinos[id], &msg, sizeof(int)*2, 1, 0);
        
        sem_wait(&mutex);

        printf("[Node %i] Received Request. Node: %i, Ticket: %i, Queue: %i \n", id, msg.nodo, msg.ticket, vecinos[msg.nodo]);

        if (max_ticket<msg.ticket) max_ticket=msg.ticket;
        if (!quiero || msg.ticket<mi_ticket || (msg.ticket==mi_ticket && msg.nodo<id )) {
            printf("[Node %i] \033[0;32mAccepted Request: Node: %i, Ticket: %i, Queue: %i\033[0m\n\n", id, msg.nodo, msg.ticket, vecinos[msg.nodo]);
            int id_node = msg.nodo;
            msg.mtype=2;
            msg.nodo=id;
            msg.ticket=0;
            msgsnd(vecinos[id_node], &msg, sizeof(int)*2 , 0);
        }else {
            pendientes[n_pendientes++]=msg.nodo;
            printf("[Node %i] New waiting node %i\n\n", id, msg.nodo);
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
    int vecinos[numNodos];
    pthread_t thread;
    pendientes=malloc ((numNodos-1) * sizeof(int));

    for (int j=0; j<numNodos;j++){
        vecinos[j]=firstq+j;
    }

    printf("[Node %i] \033[0;34mNode started. Queue: %i. Nodes: %i\033[0m\n", id, vecinos[id], numNodos);
    
    params.id=id;
    params.vecinos=vecinos;

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

        for (int i = 0; i < numNodos; i++)
        {
            if(i!=id){
                struct msgbuf msg;
                msg.mtype=1;
                msg.nodo=id;
                msg.ticket=mi_ticket;
                printf("[Node %i] \033[0;36mRequest sended: Node: %i, Ticket: %i, ToQueue: %i\033[0m \n", id, msg.nodo, msg.ticket, vecinos[i]);
                msgsnd(vecinos[i],&msg, sizeof(int)*2,0);
            }
        }
        
        printf("[Node %i] Waiting...\n", id);
        for (int i = 0; i < numNodos-1; i++)
        {
            struct msgbuf msg;    
            ssize_t x= msgrcv(vecinos[id], &msg, sizeof(int)*2, 2, 0);
            if (x==-1) printf("errorrr %i \n", vecinos[id]);            
        }

        //if (max_ticket < mi_ticket) max_ticket = mi_ticket;
        //mi_ticket++;
        
        //SECCION CRITICA
        printf("[Node %i] \033[0;31mDentro de la sección crítica.\033[0m Ticket: %i\n", id, mi_ticket);

        // Fuera de la sección crítica
        quiero=0;

        printf("[Node %i] Fuera de la sección crítica\n\n", id);

        for (int i = 0; i < n_pendientes; i++)
        {
            struct msgbuf msg;
            msg.mtype=2;
            msg.nodo=id;
            msg.ticket=0;
            printf("[Node %i] \033[0;32mSending Reply Waiting Nodes: Node: %i, Queue: %i \033[0m\n\n", id, pendientes[i], vecinos[pendientes[i]]);
            msgsnd(vecinos[pendientes[i]],&msg, sizeof(int)*2,0);
        }
        n_pendientes=0;
    }
    
    struct msgbuf msg;
    msg.mtype=3;
    msg.nodo=0;
    msg.ticket=0;
    msgsnd(vecinos[id],&msg, sizeof(int)*2,0);
    
    return 0;
}