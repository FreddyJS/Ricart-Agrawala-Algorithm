#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <math.h> //floor -> redondeo hacia bajo

#include "tickets.h" // Incluye lo necesario para las colas y la struct ticket

sem_t mutex, sc, sem_recv;

int *pendientes;
int mi_ticket;
int max_ticket=0;
int quiero=0;
int n_pendientes=0;

struct params{
	int *vecinos;
	int id;
    int idNodo;
    int ProcesosNodo;
    int numNodo;
}params;

void cont_handler() {
    sem_post(&sem_recv);
}

void init_sighandler() {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = cont_handler;

    sigaction(SIGCONT, &sigact, NULL);
}

void *receptor( void *params){
    struct params *data= (struct params *)params;

    int *vecinos= data->vecinos;
    int id= data->id;
    int idNodo= data->idNodo;
    int numNodo= data->numNodo;
    int ProcesosNodo= data->ProcesosNodo;
    int accepted = 0;
        
    ticket_t msg;
    while(1) {
        msgrcv(vecinos[idNodo/ProcesosNodo], &msg, sizeof(int)*3, 0, 0);

        if (msg.dest != id) {
            msgsnd(vecinos[idNodo/ProcesosNodo],&msg, sizeof(int)*3,0);
            continue;
        }
        
        if (msg.mtype == 2) {
            accepted++;
            if (accepted == ((numNodo*ProcesosNodo)-1)) {
                accepted = 0;
                sem_post(&sc);
            }

            continue;
        }

        int queue= (int) floor (msg.nodo/ProcesosNodo);
        int dest=msg.nodo;
        printf("[Node %i - Process %i] Received Request. Process: %i, Ticket: %i, Queue: %i \n", idNodo, id, msg.nodo, msg.ticket, vecinos[queue]);

        // Aquí movemos variables compartidas!!
        sem_wait(&mutex);

        if (max_ticket < msg.ticket) max_ticket = msg.ticket;
        if (!quiero || msg.ticket < mi_ticket || (msg.ticket == mi_ticket && msg.nodo < id )) {
            printf("[Node %i - Process %i] \033[0;32mAccepted Request: Process: %i, Ticket: %i, Queue: %i\033[0m\n\n", idNodo, id, msg.nodo, msg.ticket, vecinos[queue]);
            msg.mtype = 2;
            msg.dest = dest;
            msg.nodo = id;
            msg.ticket = 0;

            msgsnd(vecinos[queue], &msg, sizeof(int)*3 , 0);
        } else {
            pendientes[n_pendientes++]=dest;
            printf("[Node %i - Process %i] New waiting process %i\n\n", idNodo, id, dest);
        }       

        // Hemos terminado de usar variables compartidas!!
        sem_post(&mutex);
    }

    pthread_exit(NULL);
}

int main (int argc, char* argv[]){   
    sem_init(&mutex, 0, 1);
    sem_init(&sc, 0, 0);
    sem_init(&sem_recv, 0, 0);

    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int numNodos= atoi(argv[3]);
    int ProcesosNodo = atoi(argv[4]);
    int idNodo= atoi(argv[5]);
   
    int vecinos[numNodos];
    pthread_t thread;
    pendientes=malloc (((numNodos*ProcesosNodo)-1) * sizeof(int)); 

    for (int j=0; j < numNodos;j++){
        vecinos[j] = firstq+j;
    }

    printf("[Node %i - Process %i] \033[0;34mProcess started. Queue: %i. Nodes: %i\033[0m\n", idNodo, id, vecinos[idNodo/ProcesosNodo], numNodos);
    
    params.id = id;
    params.vecinos = vecinos;
    params.idNodo = idNodo;
    params.numNodo = numNodos;
    params.ProcesosNodo = ProcesosNodo;

    //init_sighandler();

    if(pthread_create(&thread, NULL, (void *)receptor, (void *)&params) != 0) {
        printf("No se ha podido crear el hilo\n"); 
        exit(0);
    }

    // Pedir entrada a la seccion critica

    while (1)
    {   
        sem_wait(&mutex);
        quiero=1;
        mi_ticket=max_ticket+1;
        sem_post(&mutex);

        for (int i = 0; i < numNodos*ProcesosNodo; i++)
        {
            if(i != id){
                ticket_t msg;
                int destQ = (int)floor(i/ProcesosNodo);
                msg.mtype = 1;
                msg.nodo = id;
                msg.ticket = mi_ticket;
                msg.dest = i;
                printf("[Node %i - Process %i] \033[0;36mRequest sended: Ticket: %i, ToQueue: %i ToProcess: %i\033[0m \n", idNodo, id, msg.ticket, vecinos[destQ], msg.dest);
                msgsnd(vecinos[destQ],&msg, sizeof(int)*3,0);

            }

        }
        
        // Esperamos por recibir las accepts
        printf("[Node %i - Process %i] Waiting...\n", idNodo, id);
        sem_wait(&sc);

        //SECCION CRITICA
        printf("[Node %i - Process %i] \033[0;31mDentro de la sección crítica.\033[0m Ticket: %i\n", idNodo, id,  mi_ticket);

        // Fuera de la sección crítica
        sem_wait(&mutex);
        quiero=0;
        sem_post(&mutex);

        // Nunca habrá un nuevo pendiente ya que quiero = 0
        printf("[Node %i - Process %i] Fuera de la sección crítica\n\n", idNodo, id);

        for (int i = 0; i < n_pendientes; i++)
        {
            ticket_t msg;
            int nodoDest = (int)floor(pendientes[i]/ProcesosNodo);
            msg.mtype = 2;
            msg.nodo = id;
            msg.ticket = 0;
            msg.dest = pendientes[i];
            printf("[Node %i - Process %i] \033[0;32mSending Reply Waiting Process: %i, Queue: %i \033[0m\n", idNodo, id, pendientes[i], vecinos[nodoDest]);
            msgsnd(vecinos[nodoDest],&msg, sizeof(int)*3,0); 
        }
        n_pendientes=0;
    }
    
    return 0;
}