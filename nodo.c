#include <stdlib.h>
#include <stdio.h>



int main (int argc, char* argv[]){
       
    int id= atoi(argv[1]);
    int firstq= atoi(argv[2]);
    int numNodos= atoi(argv[3]);
    int vecinos[numNodos-1];

    printf("id:%i first: %i num: %i \n",id,firstq,numNodos);


    for (int j=0; j<numNodos-1;j++){

        vecinos[j]=firstq+j;
        

    }





    return 0;
 
}