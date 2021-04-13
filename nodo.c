#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

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
        printf("Nodo %i : recivida req:  N%i T%i \n", id, msg.nodo, msg.ticket);

        if (max_ticket<msg.ticket) max_ticket=msg.ticket;
        if (!quiero || msg.ticket<mi_ticket || (msg.ticket==mi_ticket && msg.nodo<id )) {
            printf("Nodo %i : aceptada req:  N%i T%i \n", id, msg.nodo, msg.ticket);
            msg.mtype=2;
            msg.nodo=id;
            msg.ticket=0;
            msgsnd(vecinos[msg.nodo], &msg, sizeof(int)*2 , 0);
        }else {
            
            pendientes[n_pendientes++]=msg.nodo;
            printf("Nodo %i : nuevo nodo pendiente N%i\n", id, msg.nodo);
        }




    }


    pthread_exit(NULL);
}

int main (int argc, char* argv[]){
       
    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int numNodos= atoi(argv[3]);
    int vecinos[numNodos];
    pthread_t thread;
    pendientes=malloc ((numNodos-1) * sizeof(int));



    printf("id:%i first: %i num: %i \n",id,firstq,numNodos);


    for (int j=0; j<numNodos;j++){

        vecinos[j]=firstq+j;


    }
    params.id=id;
    params.vecinos=vecinos;

    if(pthread_create(&thread, NULL, (void *)receptor, (void *)&params)!=0) {
        printf("No se ha podido crear el hilo\n"); 
        exit(0);
    }

    //pedir entrada ala seccion critica

    quiero=1;
    mi_ticket=max_ticket+1;
    printf("Nodo %i : pedimos entrar T%i\n", id, mi_ticket);
    for (int i = 0; i < numNodos; i++)
    {
        if(i!=id){
             struct msgbuf msg;
             msg.mtype=1;
             msg.nodo=id;
             msg.ticket=mi_ticket;
             printf("Nodo %i : ENVIANDO req:  N%i T%i  A N%i \n", id, msg.nodo, msg.ticket,i);
             msgsnd(vecinos[i],&msg, sizeof(int)*2,0);
        }

    }
    
    for (int i = 0; i < numNodos-1; i++)
    {
      
        struct msgbuf msg;    
        ssize_t x= msgrcv(vecinos[id], &msg, sizeof(int)*2, 2, 0);
        if (x==-1) printf("errorrr %i \n", vecinos[id]);
        printf("Nodo %i : recivida reply  N%i\n", id, msg.nodo);
        
    }
    
    //SECCION CRITICA
    printf("Nodo %i : estamos seccion critica \n", id );
    quiero=0;

    for (int i = 0; i <n_pendientes ; i++)
    {
        struct msgbuf msg;
             msg.mtype=2;
             msg.nodo=id;
             msg.ticket=0;
        msgsnd(pendientes[i],&msg, sizeof(int)*2,0);
    }
    n_pendientes=0;
    
    struct msgbuf msg;
             msg.mtype=3;
             msg.nodo=0;
             msg.ticket=0;
        msgsnd(vecinos[id],&msg, sizeof(int)*2,0);

    return 0;
 
}