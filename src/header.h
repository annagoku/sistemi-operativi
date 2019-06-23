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

//#define DEBUG 1 //activate debug

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


//Struttura dei gruppi
typedef struct gruppo {
    int nof_elems;
    float prob;
    float min, max;
    char nof_elems_str[2];
} gruppo_t;

//Struttura dei msg inviati
typedef struct msginvite {
    long mtype; // sempre PID/matr del destinatario
    char aim; // I=invito R=rifiuto A=accettazione
    long mitt; //matr mittente msg
    int voto_AdE;
    int numCGr; // numero componenti gruppo              
} msginvite_t;

typedef struct msgFromMaster {
    long int mtype;
    int VotoSO;
    int VotoAdE;
} msgFromMaster_t;

//sefinizione del typedef del vettore della memoria condivisa
typedef struct info_shr {
    int matricola;
    int voto_AdE;
    char glab;
    char stato_s; //stato studente  F=libero, A=assigned
    char stato_g; //stato gruppo C=group closed, O=group open
    int nome_gruppo; // identificativo del gruppo finale di appartenenza
    char tipo_componente; //leader Vs follower L=leader, F=Follower
    int nof_elems;
} info_shr_t;

struct shared_data {       // struct della memoria condivisa
    unsigned long cur_idx;
    info_shr_t vec[POP_SIZE];
};

