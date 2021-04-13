#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h> // necesaria para ejecutar fork()


int NumNodos;



void deleteq (int *ID){

    for (int i=0; i<NumNodos; i++){

         msgctl(ID[i], IPC_RMID, NULL);

    }
   

}
void nodo (int id, int firstq, int numNodos){


    char param1[20] = "";
    sprintf(param1, "%i", id);
    char param2[20] = "";
    sprintf(param2, "%i", firstq);
    char param3[20] = "";
    sprintf(param3, "%i", numNodos);
    

    execl("./nodo", "./nodo", param1, param2, param3, NULL);

    exit (0);

}

int main (int argc, char* argv[]){


    NumNodos= atoi(argv[1]);
    int ID [NumNodos];

    for (int i=0; i<NumNodos; i++){
	
	    ID[i] = msgget(IPC_PRIVATE, 0777 | IPC_CREAT); 
	
	    if (ID[i] == -1){
		    printf("No se ha podido crear eñ buzón.\n");
		
            return -1;
	}
}


    for (int i=0; i<NumNodos; i++){

        pid_t child=fork();

        if (child==0) nodo(i,ID[0],NumNodos);

        

    }




    while (wait(NULL)!=-1);
    deleteq(ID);



  
}