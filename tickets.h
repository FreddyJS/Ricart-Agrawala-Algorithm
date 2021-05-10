#ifndef TICKETS_H
#define TICKETS_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

typedef struct ticket {
	long mtype; // Tipo 1 = Request 
    int ticket;
	int process;
} ticket_t;

typedef struct ticketok {
    long mtype; // Type 2 = OK
    int dest;
} ticketok_t;

#endif