#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h> 
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "tickets.h" // Incluye lo necesario para las colas y la struct ticket

sem_t mutex, sc;
sem_t *sems_mem;
ticket_t *tickets_mem;

int *pendientes;
int mi_ticket;
int max_ticket=0;
int quiero=0;
int n_pendientes=0;
int end=0;

struct params{
	int *vecinos;
	int id;
    int nodeId;
    int processPerNode;
    int numNodo;
    int type;
}params;

void cont_handler() {
    end = 1;
}

void init_sighandler() {
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = cont_handler;

    sigaction(SIGUSR1, &sigact, NULL);
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
    int mi_tipo = data->type;
        
    ticket_t request;
    ticketok_t response;

    while(!end) {
        sem_wait(&sems_mem[pos]);
        memcpy(&request, &tickets_mem[id - nodeId], sizeof(ticket_t));
        sem_post(&sems_mem[maxpos - pos]); 
        
        if (request.mtype == TICKETOK) {
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
        
        /* Not working...

        if (quiero && request.type < mi_tipo) { // win por tipo
            pendientes[n_pendientes++] = proceso_origen;
        } else if (quiero && request.type == mi_tipo && (mi_ticket < request.ticket || (mi_ticket == request.ticket && id < request.process))) { // win por id o ticket
            pendientes[n_pendientes++] = proceso_origen;
        } else {
            response.mtype = 2;
            response.dest = proceso_origen;

            msgsnd(vecinos[queue], &response, sizeof(int), 0);
        }
        */

        if (!quiero || request.ticket < mi_ticket || (request.ticket == mi_ticket && request.type > mi_tipo) || (request.ticket == mi_ticket && request.type == mi_tipo && request.process < id )) {
           response.mtype = TICKETOK;
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

    int id = atoi(argv[1]);
    int type = atoi(argv[2]);
    int firstq = atoi(argv[3]);
    int numberOfNodes = atoi(argv[4]);
    int processPerNode = atoi(argv[5]);
    int nodeId = atoi(argv[6]);
    int sems_key = atoi(argv[7]);
    int data_key = atoi(argv[8]);

    sems_mem = shmat(sems_key, NULL, 0);
    tickets_mem = shmat(data_key, NULL, 0);

    int vecinos[numberOfNodes];
    pendientes = malloc (((numberOfNodes*processPerNode)-1) * sizeof(int)); 

    for (int j=0; j < numberOfNodes;j++){
        vecinos[j] = firstq+j;
    }
    
    params.id = id;
    params.type = type;
    params.vecinos = vecinos;
    params.nodeId = nodeId;
    params.numNodo = numberOfNodes;
    params.processPerNode = processPerNode;

    pthread_t thread;
    if(pthread_create(&thread, NULL, (void *)receptor, (void *)&params) != 0) {
        printf("No se ha podido crear el hilo\n"); 
        exit(0);
    }

    while (!end)
    {   
        sem_wait(&mutex);
        quiero=1;
        mi_ticket=max_ticket+1;
        sem_post(&mutex);

        struct timeval stop_time, start_time;
        gettimeofday(&start_time, NULL);

        for (int i = 0; i < numberOfNodes; i++)
        {
            ticket_t msg;
            msg.mtype = TICKET;
            msg.process = id;
            msg.ticket = mi_ticket;
            msg.type = type;

            msgsnd(vecinos[i],&msg, sizeof(int)*3,0);
        }

        // Esperamos por recibir todos los oks
        sem_wait(&sc);
        gettimeofday(&stop_time, NULL);
        printf("\n[Node %i - Process %i] \033[0;34mWaited %lu ms\033[0m\n", nodeId/numberOfNodes, id, (stop_time.tv_sec - start_time.tv_sec)*1000 + (stop_time.tv_usec - start_time.tv_usec)/1000); 

        //SECCION CRITICA
        printf("[Node %i - Process %i] \033[0;31mDentro de la sección crítica.\033[0m Ticket: %i, Type: %i\n", nodeId/numberOfNodes, id,  mi_ticket, type);

        sleep(SCTIME);

        // Fuera de la sección crítica
        sem_wait(&mutex);
        quiero=0;
        sem_post(&mutex);
        printf("[Node %i - Process %i] \033[0;32mFuera de la sección crítica. \033[0m Ticket: %i, Type: %i\n", nodeId/numberOfNodes, id,  mi_ticket, type);

        for (int i = 0; i < n_pendientes; i++)
        {
            ticketok_t msg;
            int nodoDest = pendientes[i]/processPerNode;
            msg.mtype = TICKETOK;
            msg.dest = pendientes[i];
            
            msgsnd(vecinos[nodoDest],&msg, sizeof(int),0); 
        }
        n_pendientes=0;
    }

    free(pendientes);
    
    return 0;
}