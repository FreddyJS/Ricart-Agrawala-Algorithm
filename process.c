#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h> 
#include <string.h>
#include <unistd.h>

#include "tickets.h" // Incluye lo necesario para las colas y la struct ticket

sem_t mutex, sc, sem_recv;
sem_t *sems_mem;
ticket_t *tickets_mem;

int *pendientes;
int mi_ticket;
int max_ticket=0;
int quiero=0;
int n_pendientes=0;

struct params{
	int *vecinos;
	int id;
    int nodeId;
    int processPerNode;
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
    int nodeId= data->nodeId;
    int numNodo= data->numNodo;
    int processPerNode= data->processPerNode;
    int maxpos = processPerNode*2 -1;
    int pos = id - nodeId;
    int accepted = 0;

        
    ticket_t request;
    ticketok_t response;

    while(1) {
        sem_wait(&sems_mem[pos]);
        memcpy(&request, &tickets_mem[id - nodeId], sizeof(ticket_t));
        sem_post(&sems_mem[maxpos - pos]); 
        
        if (request.mtype == 2) {
            accepted++;
            if (accepted == ((numNodo*processPerNode)-1)) {
                accepted = 0;
                sem_post(&sc);
            }

            continue;
        }

        int queue = request.process/processPerNode;
        int proceso_origen = request.process; // proceso que pidio

        sem_wait(&mutex);

        if (max_ticket < request.ticket) max_ticket = request.ticket;
        if (!quiero || request.ticket < mi_ticket || (request.ticket == mi_ticket && request.process < id )) {
            response.mtype = 2;
            response.dest = proceso_origen;

            msgsnd(vecinos[queue], &response, sizeof(int), 0);
        } else {
            pendientes[n_pendientes++] = proceso_origen;
        }       

        sem_post(&mutex);
    }

    pthread_exit(NULL);
}

int main (int argc, char* argv[]){   
    sem_init(&mutex, 0, 1);
    sem_init(&sc, 0, 0);
    sem_init(&sem_recv, 0, 0);

    int id = atoi(argv[1]);
    int firstq = atoi(argv[2]);
    int numberOfNodes = atoi(argv[3]);
    int processPerNode = atoi(argv[4]);
    int nodeId = atoi(argv[5]);
    int sems_key = atoi(argv[6]);
    int data_key = atoi(argv[7]);

    sems_mem = shmat(sems_key, NULL, 0);
    tickets_mem = shmat(data_key, NULL, 0);

    int vecinos[numberOfNodes];
    pendientes = malloc (((numberOfNodes*processPerNode)-1) * sizeof(int)); 

    for (int j=0; j < numberOfNodes;j++){
        vecinos[j] = firstq+j;
    }
    
    params.id = id;
    params.vecinos = vecinos;
    params.nodeId = nodeId;
    params.numNodo = numberOfNodes;
    params.processPerNode = processPerNode;

    pthread_t thread;
    if(pthread_create(&thread, NULL, (void *)receptor, (void *)&params) != 0) {
        printf("No se ha podido crear el hilo\n"); 
        exit(0);
    }

    while (1)
    {   
        sem_wait(&mutex);
        quiero=1;
        mi_ticket=max_ticket+1;
        sem_post(&mutex);

        for (int i = 0; i < numberOfNodes; i++)
        {
            ticket_t msg;
            msg.mtype = 1;
            msg.process = id;
            msg.ticket = mi_ticket;

            msgsnd(vecinos[i],&msg, sizeof(int)*2,0);
        }
        
        // Esperamos por recibir todos los oks
        sem_wait(&sc);

        //SECCION CRITICA
        printf("[Node %i - Process %i] \033[0;31mDentro de la sección crítica.\033[0m Ticket: %i\n", nodeId/numberOfNodes, id,  mi_ticket);

        // Fuera de la sección crítica
        sem_wait(&mutex);
        quiero=0;
        sem_post(&mutex);
        printf("[Node %i - Process %i] \033[0;32mFuera de la sección crítica. \033[0m Ticket: %i\n", nodeId/numberOfNodes, id,  mi_ticket);

        for (int i = 0; i < n_pendientes; i++)
        {
            ticketok_t msg;
            int nodoDest = pendientes[i]/processPerNode;
            msg.mtype = 2;
            msg.dest = pendientes[i];
            
            msgsnd(vecinos[nodoDest],&msg, sizeof(int),0); 
        }
        n_pendientes=0;
    }
    
    return 0;
}