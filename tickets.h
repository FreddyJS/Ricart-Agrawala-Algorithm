#ifndef TICKETS_H
#define TICKETS_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

typedef struct ticket {
	long mtype; // Tipo 1= request, Tipo 2= reply , Tipo 3= end.
	int nodo;
    int ticket;
    int dest;
} ticket_t;


#endif