// Operating Systems: sample code  (c) Tomáš Hudec
// Race Conditions: Critical Section, Synchronization

// Assignment modified: 2022-04-01, 2022-04-12

// Zadání:
// Program spustí tři druhy vláken simulující činnosti:
// • zákazníci, kteří si rezervují vstupenky a následně je zaplatí,
// • pokladník/účetní, který nejprve čeká na dokončení rezervací,
//   následně vypíše tabulku rezervovaných lístků,
//   pak zaúčtuje platby a nakonec odešle utržené peníze majiteli,
// • majitel čeká na příjem peněz, vystaví stvrzenku a nakonec si něco koupí.
//
// Synchronizujte akce tak, aby na sebe korektně navazovaly bez zbytečného zdržování:
// • synchronní spuštění všech vláken,
// • všechny pokusy o rezervace musejí předcházet výpisu rezervací,
// • zaslání tržby majiteli musí proběhnout až po všech platbách zákazníků,
// • vydání účtenky až po předání tržby,
// • přijetí účtenky až po jejím vydání.
//
// Jednotlivé činnosti jsou reprezentovány příslušnými výpisy.
//
// Pokladník/účetní nezná počet zákazníků, ve vlákně jejich počet nelze použít.
// (Po dokončení rezervací mu však bude známý počet rezervací/plateb.)
//
// Použití aktivního čekání není přípustné.
// Pro synchronizaci použijte posixové bariéry a nepojmenované semafory.
// Pro ošetření případných kritických sekcí použijte mutexy posixových vláken.
//
// Proměnné výstižně pojmenujte dle účelu jejich použití.

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define C_NORMAL    "\033[0m"
#define C_OWNER        "\033[1;35m"
#define C_CASHIER    "\033[0;33m"
#define C_CASHIER_TAB    "\033[0;36m"
#define C_CUSTOMER    "\033[0;32m"
#define C_CUSTOMER_ERR    "\033[1;31m"

#define MAX_TICKETS    50
#define MAX_CUSTOMERS    60

int tickets = 10;            // the number of available tickets
int customers = 13;            // the number of customers

volatile int reservations[MAX_TICKETS];    // owner of each ticket, 0 = no owner
volatile int reservation_attempts = 0;    // number of finished reservation attempts
volatile int paid_tickets = 0;    // number of already paid tickets

sem_t semCashier;
sem_t semOwner;
pthread_barrier_t barrierSyncStart;
pthread_mutex_t mutexCustomer;
pthread_mutex_t mutexPaidTickets;

// release all allocated resources, used in atexit(3)
void release_resources(void) {
    sem_destroy(&semCashier);
    sem_destroy(&semOwner);
    pthread_barrier_destroy(&barrierSyncStart);
    pthread_mutex_destroy(&mutexCustomer);
    pthread_mutex_destroy(&mutexPaidTickets);
}

// synchronize start of all threads
void sync_threads() {
    pthread_barrier_wait(&barrierSyncStart);
}

// customer thread function
void *customer(int *my_id) {
    int ticket;

    sync_threads();    // synchronized start
    // printf(C_CUSTOMER "customer %2d: started" C_NORMAL "\n", *my_id);
    pthread_mutex_lock(&mutexCustomer);
    for (ticket = 0; ticket < tickets; ++ticket) {    // find the first non-reserved ticket
        if (0 == reservations[ticket]) {
            reservations[ticket] = *my_id;    // reserve
            pthread_mutex_unlock(&mutexCustomer);
            printf(C_CUSTOMER "customer %2d: reserved the ticket id %d" C_NORMAL "\n", *my_id, ticket);
            break;
        }
    }
    pthread_mutex_unlock(&mutexCustomer);

    if (ticket >= tickets) {
        fprintf(stderr, C_CUSTOMER_ERR "customer %2d: failed to reserve a ticket" C_NORMAL "\n", *my_id);
    }

    // increase number of finished reservation attempts
    pthread_mutex_lock(&mutexCustomer);
    ++reservation_attempts;
    pthread_mutex_unlock(&mutexCustomer);

    // unsuccessful reservation, return
    if (ticket >= tickets) {
        return NULL;
    }

    // reservation was successful, buy reserved ticket
    printf(C_CUSTOMER "customer %2d: sending payment for the ticket id %d" C_NORMAL "\n", *my_id, ticket);

    //increase number of already paid tickets
    pthread_mutex_lock(&mutexPaidTickets);
    paid_tickets++;
    pthread_mutex_unlock(&mutexPaidTickets);

    //all tickets paid, cashier can start accounting money
    if(paid_tickets == tickets || paid_tickets == customers){
        sem_post(&semCashier);
    }

    return NULL;
}

// cashier thread function
void *cashier(void *arg) {
    int reserved_tickets = 0;
    int t;

    sync_threads();    // synchronized start
    printf(C_CASHIER "cashier:     waiting for all reservations to finish" C_NORMAL "\n");
    sem_wait(&semCashier);
    printf(C_CASHIER "cashier:     reservation is finished" C_NORMAL "\n");

    // print all reservations
    printf(C_CASHIER_TAB "id: customer" C_NORMAL "\n");
    for (t = 0; t < tickets; ++t) {
        if (reservations[t]) {
            ++reserved_tickets;
        }
        printf(C_CASHIER_TAB "%2d: %d" C_NORMAL "\n", t, reservations[t]);
    }

    // wait for all payments and account them
    while (reserved_tickets--) {
        printf(C_CASHIER "cashier:     accounting money for a ticket" C_NORMAL "\n");
    }

    // deposit money to the owner
    printf(C_CASHIER "cashier:     going to send money to the owner" C_NORMAL "\n");
    printf(C_CASHIER "cashier:     submitting money transfer to the owner" C_NORMAL "\n");
    printf(C_CASHIER "cashier:     waiting for a receipt from the owner" C_NORMAL "\n");
    sem_post(&semOwner);
    sem_wait(&semCashier);
    printf(C_CASHIER "cashier:     got a receipt" C_NORMAL "\n");
    printf(C_CASHIER "cashier:     finished" C_NORMAL "\n");
    return NULL;
}

// owner thread function
void *owner(void *arg) {
    sync_threads();    // synchronized start
    printf(C_OWNER "owner:       waiting for money from the cashier" C_NORMAL "\n");
    sem_wait(&semOwner);
    printf(C_OWNER "owner:       got money from the cashier" C_NORMAL "\n");
    printf(C_OWNER "owner:       sending receipt to the cashier" C_NORMAL "\n");
    sem_post(&semCashier);
    printf(C_OWNER "owner:       buying a new car" C_NORMAL "\n");
    printf(C_OWNER "owner:       finished" C_NORMAL "\n");

    return NULL;
}

// main thread function
int main(int argc, char *argv[]) {
    pthread_t tid_owner;
    pthread_t tid_cashier;
    pthread_t tid_customers[MAX_CUSTOMERS];
    int customer_ids[MAX_CUSTOMERS];
    int c;

    // print id
    printf("Modified by: st64169 Píše Jiří\n");

    if (argc > 1) {
        // accept the number of tickets and customers as arguments
        errno = 0;
        tickets = strtol(argv[1], NULL, 0);
        if (errno || tickets < 1 || tickets > MAX_TICKETS) {
            fprintf(stderr, "%s: Invalid number of tickets: must be between 1 and %d.\n", argv[1], MAX_TICKETS);
            return EXIT_FAILURE;
        }
        if (argc > 2) {
            customers = strtol(argv[2], NULL, 0);
            if (errno || customers < 1 || customers > MAX_CUSTOMERS) {
                fprintf(stderr, "%s: Invalid number of customers: must be between 1 and %d.\n", argv[2], MAX_CUSTOMERS);
                return EXIT_FAILURE;
            }
        }
    }

    sem_init(&semCashier, 0, 0);
    sem_init(&semOwner, 0, 0);
    pthread_mutex_init(&mutexCustomer, NULL);
    pthread_mutex_init(&mutexPaidTickets, NULL);
    pthread_barrier_init(&barrierSyncStart, NULL, customers + 2);

    // create owner thread
    if (pthread_create(&tid_owner, NULL, owner, NULL)) {
        perror("pthread_create: cashier");
        return EXIT_FAILURE;
    }

    // create cashier thread
    if (pthread_create(&tid_cashier, NULL, cashier, NULL)) {
        perror("pthread_create: cashier");
        return EXIT_FAILURE;
    }

    // create customer threads
    for (c = 0; c < customers; ++c) {
        customer_ids[c] = c + 1;
        if (pthread_create(&tid_customers[c], NULL, (void *(*)(void *)) customer, &customer_ids[c])) {
            perror("pthread_create: customer");
            return EXIT_FAILURE;
        }
    }
    // join all threads: customers, cashier, owner
    for (c = 0; c < customers; ++c) {
        (void) pthread_join(tid_customers[c], NULL);
    }
    (void) pthread_join(tid_cashier, NULL);
    (void) pthread_join(tid_owner, NULL);
    // Errors not checked, reasons why error cannot happen:
    // EDEADLK	only main thread uses join
    // EINVAL	all threads are joinable
    // ESRCH	all thread ids are valid

    return EXIT_SUCCESS;
}