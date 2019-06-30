#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <ctype.h>


#define POP_SIZE  300 // num studenti
#define MAX_REJECT 2 // num max rifiuti
#define SIM_TIME 15 // tempo di simulazione

//codici identificativi IPC
#define SEM_ID  70500
#define SHR_ID  70651
#define MSG_ID  7190
#define MSG_MASTER 7200

#define INIT_READY 0 //semaforo per verificare la fine dell'inizializzazione dei figli
#define SHR_SCRIPT 1 //semaforo per scrittura su memoria condivisa


//Struttura dei msg inviati
typedef struct msginvite {
    long mtype; // sempre PID/matr del destinatario
    char aim; // I=invito R=rifiuto A=accettazione
    long mitt; //PID/matr mittente msg
    int voto_AdE;
    int numCGr; // numero componenti gruppo desiderato dal mittente            
} msginvite_t;

typedef struct msgFromMaster {
    long int mtype;
    int VotoSO;
    int VotoAdE;
} msgFromMaster_t;

//sefinizione del typedef degli elementi del vettore della memoria condivisa
typedef struct info_shr {
    int matricola;
    int voto_AdE;
    char glab; //turno di laboratorio T4 per matr pari--T3 matr dispari
    char stato_s; //stato studente  F=libero, A=assigned
    char stato_g; //stato gruppo C=group closed, O=group open
    int nome_gruppo; // identificativo del gruppo finale di appartenenza uguale al PID/matr del leader
    char tipo_componente; //leader Vs follower L=leader, F=Follower
    int nof_elems;// numero elementi desiderati per il proprio gruppo
} info_shr_t;

// struct della memoria condivisa
struct shared_data {       
    unsigned long cur_idx; //contatore di posizione nel vettore della memoria condivisa
    info_shr_t vec[POP_SIZE];
};

