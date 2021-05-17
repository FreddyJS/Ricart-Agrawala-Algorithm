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

struct pendientes *pendientes;
int n_pendientes=0;
int max_ticket=0;
int extra_oks;
int mi_ticket;
int quiero=0;
int dentro=0;
int end=0;

struct pendientes
{
    int node;
    int process;
};

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
    int mi_tipo = data->type;

    int maxpos = processPerNode*2 -1;
    int accepted = 0;
        
    ticket_t request;
    ticketok_t response;
    int pos = id;

    while(!end) {
        sem_wait(&sems_mem[pos]);
        memcpy(&request, &tickets_mem[pos], sizeof(ticket_t));
        sem_post(&sems_mem[maxpos - pos]); 
        
        if (request.mtype == TICKETOK) {
            memcpy(&response, &request, sizeof(ticketok_t));

            /* mximum priority
            if (response.org_node != -1) {
                pendientes[n_pendientes].node = response.org_node;
                pendientes[n_pendientes].process = response.org_process;  
                n_pendientes++;          
            }
            */

            accepted++;
            if (accepted == ((numNodo*processPerNode)-1 +extra_oks)) {
                accepted = 0;
                extra_oks = 0;
                sem_post(&sc);
            }

            continue;
        }

        int queue = request.node;
        sem_wait(&mutex);

        if (max_ticket < request.ticket) max_ticket = request.ticket;

        /* maximum priority */
        /*
        if (!dentro && request.type > mi_tipo) {
            response.mtype = TICKETOK;
            response.dest = request.process;
            response.org_process = -1;
            response.org_node = -1;

            if(quiero) {
                response.org_node = nodeId; 
                response.org_process = id;
                extra_oks++;
            }

            msgsnd(vecinos[queue], &response, sizeof(int)*3, 0);

        } else if (dentro || (quiero && request.type < mi_tipo)) {
            pendientes[n_pendientes].node = request.node;
            pendientes[n_pendientes].process = request.process;
            n_pendientes++;

        } else if (dentro || (quiero && (
                    (request.type == mi_tipo && mi_ticket<request.ticket) || 
                    (request.type==mi_tipo && mi_ticket==request.ticket && nodeId<request.node) || 
                    request.type==mi_tipo && mi_ticket==request.ticket && nodeId==request.node && id<request.process)))
        {
            pendientes[n_pendientes].node = request.node;
            pendientes[n_pendientes].process = request.process;
            n_pendientes++;

        } else {
            response.mtype = 2;
            response.dest = request.process;
            response.org_process = -1;
            response.org_node = -1;

            if (quiero && request.type > mi_tipo) {
                response.org_process = id;
                response.org_node = nodeId;
            }

            msgsnd(vecinos[queue], &response, sizeof(int)*3, 0);
        }
        */

        /* Desempatando por ticket */
        if (!quiero || request.ticket < mi_ticket || 
            (request.ticket == mi_ticket && request.type > mi_tipo) || 
            (request.ticket == mi_ticket && request.type == mi_tipo && request.node < nodeId) || 
            (request.ticket == mi_ticket && request.type == mi_tipo && request.node == nodeId && request.process < id )) 
        {
           // Aceptando peticion
           //printf("[Node %i - Process %i] \033[0;32mAccepted:\033[0m Ticket %i, Tipo %i, Node %i, Process %i\n", nodeId, id, request.ticket, request.type, request.node, request.process);
           response.mtype = TICKETOK;
           response.dest = request.process;

           msgsnd(vecinos[queue], &response, sizeof(int), 0);
        } else {
            pendientes[n_pendientes].node = request.node; 
            pendientes[n_pendientes].process = request.process;
            n_pendientes++;
            //printf("[Node %i - Process %i] \033[0;36mPendiente:\033[0m Ticket %i, Tipo %i, Node %i, Process %i (%i)\n", nodeId, id, request.ticket, request.type, request.node, request.process, n_pendientes);
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
    pendientes = malloc (((numberOfNodes*processPerNode)-1) * sizeof(struct pendientes)); 

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


#ifdef SYNCTIME
    while (1)
    {
        if (nodeId == numberOfNodes-1 && id == processPerNode-1) break;
    }
#endif

    unsigned long waited;

#ifndef SYNCTIME
    while (!end)
    {  
#endif 
        if (type == ANULACIONES)
            sleep(10);
        sem_wait(&mutex);
        quiero=1;
        mi_ticket=max_ticket+1;
        sem_post(&mutex);

        struct timeval stop_time, start_time;
        gettimeofday(&start_time, NULL);

        // Enviamos todas las peticiones
        //printf("[Node %i - Process %i] There are %i nodes\n", nodeId, id, numberOfNodes);
        ticket_t msg;
        msg.mtype = TICKET;
        msg.node = nodeId;
        msg.process = id;
        msg.ticket = mi_ticket;
        msg.type = type;

        for (int i = 0; i < numberOfNodes; i++)
        {
            msgsnd(vecinos[i],&msg, sizeof(int)*4,0);
        }

        // Esperamos por recibir todos los oks
        sem_wait(&sc);
        sem_wait(&mutex);
        dentro = 1;
        sem_post(&mutex);

        gettimeofday(&stop_time, NULL);
        waited = (stop_time.tv_sec - start_time.tv_sec)*1000 + (stop_time.tv_usec - start_time.tv_usec)/1000;

        if (type == ANULACIONES)
            printf("\n[Node %i - Process %i] \033[0;34mWaited %lu ms\033[0m\n", nodeId, id, waited); 

        //SECCION CRITICA
        if (type == ANULACIONES)
            printf("[Node %i - Process %i] \033[0;31mDentro de la sección crítica.\033[0m Ticket: %i, Type: %i\n", nodeId, id,  mi_ticket, type);

        sleep(SCTIME);

        // Fuera de la sección crítica
        if (type == ANULACIONES)
            printf("[Node %i - Process %i] \033[0;32mFuera de la sección crítica. \033[0m Ticket: %i, Type: %i\n", nodeId, id,  mi_ticket, type);
        
        sem_wait(&mutex);
        dentro=0;
        quiero=0;
        sem_post(&mutex);

        ticketok_t msgok;
        if (type == ANULACIONES)
            printf("[Node %i - Process %i] \033[0;32mPendientes:\033[0m %i\n", nodeId, id, n_pendientes);

        for (int i = 0; i < n_pendientes; i++)
        {
            int nodoDest = pendientes[i].node;
            msgok.mtype = TICKETOK;
            msgok.dest = pendientes[i].process;
            //msgok.org_node = -1;
            //msgok.org_process = -1;
            //printf("[Node %i - Process %i] \033[0;32mAceppted:\033[0m Node %i, Process %i\n", nodeId, id, pendientes[i].node, pendientes[i].process);
            
            msgsnd(vecinos[nodoDest],&msgok, sizeof(int),0); 
        }
        n_pendientes=0;

        if (type == ANULACIONES) break;
#ifndef SYNCTIME
    }
#endif

    free(pendientes);

    char buffer[55];
    sprintf(buffer, "%lu", waited);

    FILE *logfile;
    char fileName[55];
    sprintf(fileName, "logs/inercia%in%ip.log", numberOfNodes, processPerNode);
    logfile = fopen(fileName, "a");
    fprintf(logfile, "%s\n", buffer);
    fclose(logfile);

#ifdef SYNCTIME
    // justo al acabar 0 delay
#endif
    
    return 0;
}