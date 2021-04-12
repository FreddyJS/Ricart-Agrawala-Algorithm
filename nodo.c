#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>




void *receptor( void *vecinos){

    pthread_exit(NULL);
}

int main (int argc, char* argv[]){
       
    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int numNodos= atoi(argv[3]);
    int vecinos[numNodos-1];
    pthread_t thread;

    printf("id:%i first: %i num: %i \n",id,firstq,numNodos);


    for (int j=0; j<numNodos-1;j++){

        vecinos[j]=firstq+j;


    }

    if(pthread_create(&thread, NULL, (void *)receptor, (void *)vecinos)!=0) {
        printf("No se ha podido crear el hilo\n"); 
        exit(0);
    }

    //pedir entrada ala seccion critica





    return 0;
 
}