#ifndef TICKETS_H
#define TICKETS_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define SCTIME 1

#define TICKET 1
#define TICKETOK 2

#define ANULACIONES 6
#define PAGOS 5
#define ADMIN 4
#define PRERESERVAS 3
#define GRADAS 2
#define EVENTOS 1

typedef struct ticket {
	long mtype; // Tipo 1 = Request 
    int ticket;
    int node;
	int process;
    int type;
} ticket_t;

typedef struct ticketok {
    long mtype; // Type 2 = OK
    int dest;
    int org_node;
    int org_process;
} ticketok_t;

#endif