#include "header.h"

struct studente_t{
    int matricola;
    int voto_AdE;
    int nof_elems;
    char glab; //gruppo laboratorio
};

int sem_init; // identificatore del vettore di semafori
int shr_mem; //identificatore della memoria condivisa
int msg_id; //identificatore della coda di messaggi
int msg_master; //identificatore della coda di messaggi master finale
int volatile flagSignal=0;
int preAlarm = 0;
struct shared_data *project_data; //puntatore a memoria condivisa 

void    sigHandler (int sign);
int     inviteStudents (int shr_pos, struct studente_t *studente, int vmin, int vmax, int nof_invites);
int     acceptInvite(int shr_pos, struct studente_t *studente, msginvite_t *rec);
int     rejectInvite(int shr_pos, struct studente_t *studente, msginvite_t *rec);

int main(int argc, char **argcv) {
    struct studente_t studente;
    struct sembuf presenza; //struttura dati di sistema per i semafori 
    struct sembuf w_dati;
    int shr_pos; //posizione propria nel vettore di memoria condivisa
    struct sigaction ends; // strutture dati di sistema per la gestione dei segnali
    sigset_t smask;
    //variabili per la coda dei messaggi
    //msginvite_t invito;
    msginvite_t received;
    msginvite_t answer;
    msgFromMaster_t votoFinale;
    int nof_invites =0;
    int count_rjt=0; //contatore numero rifiuti
    int exitStrategy = 0;
    int fate =0;

    pid_t pid;


    
    
    pid = getpid();
    if(argcv != NULL) {
        studente.nof_elems = atoi(argcv[1]);
        nof_invites = atoi(argcv[2]);
        //printf("[Studente %d] max no invites %d\n",pid, nof_invites);
    }

    //printf ("Studente pid %d\n", pid);
    
    //per comodita' imposto la matricola uguale al pid
    studente.matricola = pid;
    received.mtype=studente.matricola;

    //stabilisco il gruppo dello studente in base alla matricola
    if(studente.matricola%2 == 0) {
        studente.glab = '4';
    }
    else {
        studente.glab = '3';
    }
    
    //recupero semaforo
    if ((sem_init=semget(SEM_ID, 2, 0600))==-1){
        printf("[Studente %d] Errore nel recupero del vettore di semafori \n ",studente.matricola);
        exit(-2);
    }
    //recupero memoria condivisa
    if((shr_mem=shmget(SHR_ID, sizeof(&project_data), 0600))<0 ){
        printf("[Studente %d] Errore nel recupero della memoria condivisa \n ",studente.matricola);
        exit(-2);
    }
    project_data=shmat(shr_mem, NULL, 0);
    //recupero coda di messaggi
    if((msg_id=msgget(MSG_ID, 0600))<0 ){
        printf("[Studente %d] Errore nel recupero della coda di messaggi \n ",studente.matricola);
        perror("msgget");
        exit(-2);
    }

    //recupero coda di messaggi master finale
    if((msg_master=msgget(MSG_MASTER, 0600))<0 ){
        printf("[Studente %d] Errore nel recupero della coda di messaggi \n ",studente.matricola);
        perror("msgget");
        exit(-2);
    }
    //LOCK SHARED MEMORY
    w_dati.sem_flg=0;
    w_dati.sem_num=SHR_SCRIPT;
    w_dati.sem_op=-1;
    semop(sem_init, &w_dati, 1);

    
    shr_pos=project_data->cur_idx;
    // inizializzo prendendo come seme l'indice
    srand(shr_pos); 
    
    // il voto lo prendo come uniforme tra 18 e 30
    studente.voto_AdE = 18 + rand() % (30 -18 +1);
    #ifdef DEBUG
    printf("[Studente %d - lab %c - i:%d] Voto %d, pref. gruppo %d, lab. %c\n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE, studente.nof_elems, studente.glab);
    
    printf("[Studente %d - lab %c - i:%d] Scrivo in mem condivisa \n",studente.matricola, studente.glab, shr_pos);
    #endif
    
    project_data->vec[shr_pos].matricola=studente.matricola;
    project_data->vec[shr_pos].voto_AdE=studente.voto_AdE;
    project_data->vec[shr_pos].glab=studente.glab;
    project_data->vec[shr_pos].stato_s='F';
    project_data->vec[shr_pos].stato_g=' ';
    project_data->vec[shr_pos].nome_gruppo=0; // inizializzato a 0 per tutti 
    project_data->vec[shr_pos].tipo_componente=' ';
    project_data->vec[shr_pos].nof_elems=studente.nof_elems; //preferenza elementi del gruppo
    project_data->cur_idx++;

    //Sblocco semaforo presenza
    presenza.sem_num=INIT_READY;
    presenza.sem_op=1;
    #ifdef DEBUG
    printf("[Studente %d] Sblocco semaforo presenza \n",studente.matricola);
    #endif
    
    semop(sem_init, &presenza, 1);

    //UNLOCK
    w_dati.sem_flg=0;
    w_dati.sem_num=SHR_SCRIPT;
    w_dati.sem_op=1;
    #ifdef DEBUG
    printf("[Studente %d] Sblocco semaforo \n",studente.matricola);
    #endif

    semop(sem_init, &w_dati, 1);

    //Gestione del risveglio
    ends.sa_handler=sigHandler;
    ends.sa_flags=0;
    sigemptyset(&smask);
    ends.sa_mask=smask;
    sigaction (SIGUSR1, &ends, NULL);

    //Gestione del prealarm
    ends.sa_handler=sigHandler;
    ends.sa_flags=0;
    sigemptyset(&smask);
    ends.sa_mask=smask;
    sigaction (SIGUSR2, &ends, NULL);  
    
    // Metto in pausa in attesa del segnale di inizio di formazione gruppi
    #ifdef DEBUG
    printf("[Studente %d] Metto in pausa\n",studente.matricola);
    #endif

    pause();

    #ifdef DEBUG
    printf("[Studente %d - lab %c - i:%d] Risveglio\n",studente.matricola, studente.glab, shr_pos);
    #endif
    //Gestione del segnale di ALARM
    ends.sa_handler=sigHandler;
    ends.sa_flags=0;
    sigemptyset(&smask);
    ends.sa_mask=smask;
    sigaction (SIGALRM, &ends, NULL);

    

    if (studente.voto_AdE>=18 && studente.voto_AdE<=21){
        while (exitStrategy== 0 && flagSignal==0){
            //gli studenti con voti bassi si mettono solo in ricezione inviti
            #ifdef DEBUG
            printf("[Studente %d - lab %c - i:%d] (%d) mi metto in ricezione (stato %c) \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE, project_data->vec[shr_pos].stato_s);
            #endif

            if(msgrcv(msg_id, &received, sizeof(received)-sizeof(long), studente.matricola, 0)==-1){
                if(errno==EINTR){ // interruzione da segnale
                    //fine del SIM_TIME
                    if(flagSignal == 0 && preAlarm == 1) {
                        // allora è stato ricevuto un preAlarm
                        // se sono ancora libero decido di chiudere il gruppo da solo
                        if(project_data->vec[shr_pos].stato_s=='F') {
                            #ifdef DEBUG
                            printf("[Studente %d - lab %c - i:%d] (%d) Ricevuto preAlarm -> chiudo il gruppo da solo \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE);
                            #endif
                            project_data->vec[shr_pos].stato_g='C';
                            project_data->vec[shr_pos].stato_s='A';
                            project_data->vec[shr_pos].tipo_componente='L';
                            project_data->vec[shr_pos].nome_gruppo=studente.matricola;
                            
                            
                        } 

                    }
                    else {
                        #ifdef DEBUG
                        printf("[Studente %d - lab %c - i:%d] Errore nella ricezione dei messaggi \n", studente.matricola, studente.glab, shr_pos);
                        #endif

                        exitStrategy =1;
                        break;
                    }
                }
            }
            else{
                if (received.aim=='I'){

                    if (project_data->vec[shr_pos].stato_s!='F'){
                        //rifiuto messaggio
                        if(rejectInvite(shr_pos, &studente, &received)==-1){
                            if(errno==EINTR){ // interruzione da segnale
                                //fine del SIM_TIME
                                if(flagSignal == 0 && preAlarm == 1) {
                                    // allora è stato ricevuto un preAlarm
                                    // devo riprovare a rifiutare  il messaggio
                                    if(rejectInvite(shr_pos, &studente, &received)==-1){
                                        #ifdef DEBUG
                                        printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                        #endif
                                        exitStrategy =1;
                                        break;    
                                    }

                                }
                                else {
                                    #ifdef DEBUG
                                    printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                    #endif
                                    exitStrategy =1;
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        //accetto il messaggio
                        if(acceptInvite(shr_pos, &studente, &received)==-1){
                            if(flagSignal == 0 && preAlarm &&  errno==EINTR) {
                                // allora è stato ricevuto il preAlarm
                                // devo ritentare di accettare il messaggio
                                if(acceptInvite(shr_pos, &studente, &received)==-1){
                                    // se sono qui allora è successo un nuovo errore 
                                    // devo uscire
                                    #ifdef DEBUG
                                    printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta\n", studente.matricola, studente.glab, shr_pos);
                                    #endif

                                    break;
                                }        
                            }
                            else if(flagSignal==1 && errno==EINTR){ // interruzione da segnale
                                //fine del SIM_TIME
                                exitStrategy =1;
                                #ifdef DEBUG
                                printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta\n", studente.matricola, studente.glab, shr_pos);
                                #endif
                                break;
                            }
                            
                        }
                    }
                    
                }
                else {
                    #ifdef DEBUG
                    printf("[Studente %d - lab %c - i:%d] (%d) messaggio non previsto %d %ld\n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE, received.aim,  received.mitt);
                    #endif
                    exitStrategy = 1;
                    break;
                }
            }
            
        }

    } // fine if studenti 18-21
    else if (studente.voto_AdE>=22 && studente.voto_AdE<=27){
        
        while (exitStrategy== 0 && flagSignal==0 ){


            if(preAlarm == 1 && project_data->vec[shr_pos].stato_s=='F') {
                #ifdef DEBUG
                printf("[Studente %d - lab %c - i:%d] (%d) Ricevuto preAlarm -> chiudo il gruppo da solo \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE);
                #endif

                project_data->vec[shr_pos].stato_g='C';
                project_data->vec[shr_pos].stato_s='A';
                project_data->vec[shr_pos].tipo_componente='L';
                project_data->vec[shr_pos].nome_gruppo=studente.matricola;
                            
            }

            //gli studenti con voti medi si mettono prima in ricezione inviti
            //printf("[Studente %d] (22-27) mi metto in ricezione \n", studente.matricola);
            
            if(msgrcv(msg_id, &received, sizeof(received)-sizeof(long), studente.matricola, IPC_NOWAIT)==-1){
                if(errno==EINTR){ // interruzione da segnale
                    if(flagSignal == 0 && preAlarm == 1) {
                        // allora è stato ricevuto un preAlarm
                        // se sono ancora libero decido di chiudere il gruppo da solo
                        if(project_data->vec[shr_pos].stato_s=='F') {
                            #ifdef DEBUG
                            printf("[Studente %d - lab %c - i:%d] (%d) Ricevuto preAlarm -> chiudo il gruppo da solo \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE);
                            #endif
                            project_data->vec[shr_pos].stato_g='C';
                            project_data->vec[shr_pos].stato_s='A';
                            project_data->vec[shr_pos].tipo_componente='L';
                            project_data->vec[shr_pos].nome_gruppo=studente.matricola;
                            
                        }
                    }
                    else {
                        //fine del SIM_TIME
                        #ifdef DEBUG
                        printf("[Studente %d - lab %c - i:%d] Errore nella ricezione dei messaggi \n", studente.matricola, studente.glab, shr_pos);
                        #endif                
                        exitStrategy =1;
                        break;
                    }
                }
                else if (errno==ENOMSG){
                    if( project_data->vec[shr_pos].stato_s=='F') {
                        if(preAlarm == 1 && project_data->vec[shr_pos].stato_s=='F') {
                            #ifdef DEBUG
                            printf("[Studente %d - lab %c - i:%d] (%d) Ricevuto preAlarm -> chiudo il gruppo da solo \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE);
                            #endif                                    
                            project_data->vec[shr_pos].stato_g='C';
                            project_data->vec[shr_pos].stato_s='A';
                            project_data->vec[shr_pos].tipo_componente='L';
                            project_data->vec[shr_pos].nome_gruppo=studente.matricola;
                        }
                        else {
                            //printf("[Studente %d] (%d) Non ho messaggi e sono libero... controllo il fato \n", studente.matricola, studente.voto_AdE);
                            fate = 1 + rand() % 2;

                            if(fate==1 && studente.voto_AdE >= 24) {
                                if(inviteStudents(shr_pos, &studente, 22, 27, nof_invites)==-1) {
                                    break;
                                }
                            }
                        }
                    }
                }
                else{
                    //deve essere successi un errore
                    perror("msgrcv");
                    
                }
                
            }
            else {
                #ifdef DEBUG
                printf("[Studente %d - lab %c - i:%d] (%d) ricevuto invito da %ld, tipo %c\n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE, received.mitt, received.aim);
                #endif
                if (project_data->vec[shr_pos].stato_s!='F'){
                    //rifiuto messaggio
                    if(rejectInvite(shr_pos, &studente, &received)==-1){
                        
                        if(errno==EINTR){ // interruzione da segnale
                            //fine del SIM_TIME
                            if(flagSignal == 0 && preAlarm == 1) {
                                // allora è stato ricevuto un preAlarm
                                // devo riprovare a rifiutare  il messaggio
                                if(rejectInvite(shr_pos, &studente, &received)==-1){
                                    printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                    exitStrategy =1;
                                    break;    
                                }

                            }
                            else {
                                printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                exitStrategy =1;
                                break;
                            }
                        }
                    }
                }
                else {
                    if(received.aim=='I') {
                        if(count_rjt<MAX_REJECT && received.numCGr!=studente.nof_elems) {
                            //provo a vedere se trovo un gruppo migliore
                            count_rjt++;
                            //rifiuto il messaggio
                            //printf("[Studente %d - lab %c - i:%d] (%d) rifiuto l'invito di %ld \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE, received.mitt);
            
                            if(rejectInvite(shr_pos, &studente, &received)==-1){
                                if(errno==EINTR){ // interruzione da segnale
                                    //fine del SIM_TIME
                                    if(flagSignal == 0 && preAlarm == 1) {
                                        // allora è stato ricevuto un preAlarm
                                        // mi conviene accettare a questo punto invece di rimanere 
                                        if(acceptInvite(shr_pos, &studente, &received)==-1){
                                            if(flagSignal==1 && errno==EINTR){ // interruzione da segnale
                                                //fine del SIM_TIME
                                                exitStrategy =1;
                                                break;
                                            }
                                            printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                        }

                                    }
                                    else {
                                        printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                        exitStrategy =1;
                                        break;
                                    }
                                }                
                            }
                            else {
                                //genero un numero casuale per decidere se propormi oppure se vedere se mi arrivano inviti migliori

                                fate = 1 + rand() % 2;

                                if(fate==1 && studente.voto_AdE >= 24 ) {
                                    if(inviteStudents(shr_pos, &studente, 22, 27, nof_invites)==-1) {
                                        break;
                                    }
                                }
                            }

                        }
                        else {
                            //accetto il messaggio
                            if(acceptInvite(shr_pos, &studente, &received)==-1){
                                if(flagSignal == 0 && preAlarm &&  errno==EINTR) {
                                    // allora è stato ricevuto il preAlarm
                                    // devo ritentare di accettare il messaggio
                                    if(acceptInvite(shr_pos, &studente, &received)==-1){
                                        // se sono qui allora è successo un nuovo errore 
                                        // devo uscire
                                        printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta\n", studente.matricola, studente.glab, shr_pos);
                                        break;
                                    }        
                                }
                                else if(flagSignal==1 && errno==EINTR){ // interruzione da segnale
                                    //fine del SIM_TIME
                                    exitStrategy =1;
                                    #ifdef DEBUG
                                    printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta\n", studente.matricola, studente.glab, shr_pos);
                                    #endif
                                    break;
                                }
                    
                            }
                        }
                        
                        
                    } 
                    else {
                        if(preAlarm == 1 && project_data->vec[shr_pos].stato_s=='F') {
                            printf("[Studente %d - lab %c - i:%d] (%d) Ricevuto preAlarm -> chiudo il gruppo da solo \n", studente.matricola, studente.glab, shr_pos, studente.voto_AdE);
                                        
                            project_data->vec[shr_pos].stato_g='C';
                            project_data->vec[shr_pos].stato_s='A';
                            project_data->vec[shr_pos].tipo_componente='L';
                            project_data->vec[shr_pos].nome_gruppo=studente.matricola;
                        }
                        
                    }       
                }
            }
        }

        
    } // fine if studenti 22-27
    else {
        
        // studenti con voto alto
        //printf("[Studente %d] (28-30) controllo se mi sono arrivati messaggi\n", studente.matricola);

        while (exitStrategy== 0 && flagSignal==0) {

            if (project_data->vec[shr_pos].stato_s!='F'){
                if(msgrcv(msg_id, &received, sizeof(received)-sizeof(long), studente.matricola, 0)==-1){
                    //exitStrategy=1;
                    //break;
                }
                else {
                    //rifiuto messaggio
                    if(rejectInvite(shr_pos, &studente, &received)==-1){
                        if(errno==EINTR){ // interruzione da segnale
                            //fine del SIM_TIME
                            if(flagSignal == 0 && preAlarm == 1) {
                                // allora è stato ricevuto un preAlarm
                                // devo riprovare a rifiutare  il messaggio
                                if(rejectInvite(shr_pos, &studente, &received)==-1){
                                    printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                      
                                }

                            }
                            else if(flagSignal == 1) {
                                printf("[Studente %d - lab %c - i:%d] Errore nell'invio della risposta \n", studente.matricola, studente.glab, shr_pos);
                                exitStrategy =1;
                                break;
                            }
                        }
                    }
                }
                
            } 
            else {
                if(inviteStudents(shr_pos, &studente, 18, 21, nof_invites)==-1) {
                    break;
                }
            }

           
        }

        
            
            
            
    }//fine if studenti con voti alti





    
    if(msgrcv(msg_master, &votoFinale, sizeof(votoFinale)-sizeof(long), studente.matricola, 0)==-1){
        printf("[Studente %d - lab %c - i:%4d] Errore nella ricezione del voto finale \n", studente.matricola, studente.glab, shr_pos);
        
    } 
    else{
        
        printf("[Studente %d - lab %c - i:%4d] Voto di AdE %2d Voto Sistemi Operativi %2d\n", studente.matricola, studente.glab, shr_pos, votoFinale.VotoAdE, votoFinale.VotoSO);
        
    }
    #ifdef DEBUG
    printf("[Studente %d - lab %c - i:%d] exit 0\n",studente.matricola, studente.glab, shr_pos);
    #endif
    exit(0);
}

// gestione segnali 
void sigHandler (int sign){
    if(sign==SIGUSR1) {
        //risveglio
    }
    else if (sign == SIGUSR2) {
        preAlarm=1;
    }
    else if (sign == SIGALRM) {
        
            flagSignal=1;
    }
    else if (sign==SIGINT){

        raise(SIGKILL);
    }
    
}

int inviteStudents (int shr_pos, struct studente_t *studente, int vmin, int vmax, int nof_invites){
    int j=0;
    int count_elem=1;
    int count_inv=0;
    int found = 0;
    

    msginvite_t invito;
    msginvite_t received;
    

    project_data->vec[shr_pos].nome_gruppo=studente->matricola;
    project_data->vec[shr_pos].tipo_componente='L';
    project_data->vec[shr_pos].stato_s='A';

    
    //decido di propormi come leader
    #ifdef DEBUG
    printf("[Studente %d - lab %c - i:%d] (%d) Si propone come leader \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE);
    #endif
    
    for (j=0; j<POP_SIZE && count_elem<=studente->nof_elems && count_inv<nof_invites  && preAlarm == 0; j++){
        if( project_data->vec[j].stato_s=='F' && 
            project_data->vec[j].glab==studente->glab && 
            project_data->vec[j].voto_AdE>=vmin &&  
            project_data->vec[j].voto_AdE<=vmax){ 

            if((studente->nof_elems >=27 && project_data->vec[j].nof_elems==studente->nof_elems) || 
               (studente->nof_elems <27 && ((j<POP_SIZE/2 && project_data->vec[j].nof_elems==studente->nof_elems)||j>=POP_SIZE/2) )) {//rilassamento della condizione numerosita' dopo POP_SIZE/2
                invito.mtype=project_data->vec[j].matricola;
                invito.aim='I';
                invito.mitt=studente->matricola;
                invito.voto_AdE=studente->voto_AdE;
                invito.numCGr=studente->nof_elems;

                #ifdef DEBUG
                printf("[Studente %d - lab %c - i:%d] (%d) Mando invito a %ld \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE, invito.mtype);
                #endif

                if(msgsnd (msg_id, &invito, sizeof(invito)-sizeof(long),0)==-1){
                        printf("[Studente %d - lab %c - i:%d] Errore nell'invio del messaggio\n", studente->matricola, studente->glab, shr_pos);
                        return -1;    
                }
                else{
                    count_inv++;
                }
                #ifdef DEBUG
                printf("[Studente %d - lab %c - i:%d] (%d) count inviti %d \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE,count_inv);
                printf("[Studente %d - lab %c - i:%d] (%d) Mi metto in ricezione (per accettazione) \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE);
                #endif
                found = 0;

                while (found == 0) {
                    if(msgrcv(msg_id, &received, sizeof(invito)-sizeof(long), studente->matricola, 0)==-1){
                        //controllo per quale motivo mi sono bloccato perché se è per un segnale devo controllare che non sia il prealarm
                        if(errno==EINTR){ // interruzione da segnale
                            //fine del SIM_TIME
                            if(flagSignal == 0 && preAlarm == 1) {
                                // allora è stato ricevuto un preAlarm
                                // devo riprovare ad aspettare che arrivi il messaggio di riposta ma se ricevo un rifiuto
                                // chiudo comunque il gruppo
                                #ifdef DEBUG
                                printf("[Studente %d - lab %c - i:%d] Errore nella ricezione perché ho ricevuto un preAlarm -> continuo a cercare di ricevere feedback dell'invito\n", studente->matricola, studente->glab, shr_pos);
                                #endif

                            }
                            else {
                                printf("[Studente %d - lab %c - i:%d] Errore nella ricezione del messaggio\n", studente->matricola, studente->glab, shr_pos);
                                return -1;
                            }
                        }
                        else {
                            printf("[Studente %d - lab %c - i:%d] Errore nella ricezione del messaggio\n", studente->matricola, studente->glab, shr_pos);
                            return -1; 
                        }    
                    }
                    else if(received.aim=='A'){
                        #ifdef DEBUG
                        printf("[Studente %d - lab %c - i:%d] (%d) il mio invito è stato accettato da %ld \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE, received.mitt);
                        #endif
                        found = 1;
                        count_elem++; 
                        
                    }
                    else if(received.aim=='R'){
                        #ifdef DEBUG
                        printf("[Studente %d - lab %c - i:%d] Il mio invito è stato rifiutato  da %ld, tipo %c \n", studente->matricola, studente->glab, shr_pos, received.mitt, received.aim);
                        #endif
                        found =1;
                    }
                    else if(received.aim=='I'){
                        #ifdef DEBUG
                        printf("[Studente %d - lab %c - i:%d] Ricevuto invito da %ld, tipo %d \n", studente->matricola, studente->glab, shr_pos, received.mitt, received.aim);
                        #endif

                        if(rejectInvite(shr_pos, studente, &received)==-1) {
                            printf("[Studente %d - lab %c - i:%d] Errore nel rifiuto del messaggio di %ld \n", studente->matricola, studente->glab, shr_pos, received.mitt);
                            
                        }
                    }
                }
            }

        
        }        
    } 
    project_data->vec[shr_pos].stato_g='C';
    #ifdef DEBUG
    printf("[Studente %d - lab %c - i:%d] (%d) chiudo il gruppo \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE);
    #endif
    return 0;
}

int acceptInvite(int shr_pos, struct studente_t *studente, msginvite_t *rec) {
    msginvite_t answer;

    project_data->vec[shr_pos].stato_s='A';
    project_data->vec[shr_pos].nome_gruppo=rec->mitt; // inizializzato a 0 per tutti 
    project_data->vec[shr_pos].tipo_componente='F';
    //invia msg di accettazione ed esce

    answer.mtype=rec->mitt;
    answer.mitt=studente->matricola;
    answer.aim='A';
    #ifdef DEBUG
    printf("[Studente %d - lab %c - i:%d] (%d) accetto invito di %ld \n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE, rec->mitt);
    #endif
    return msgsnd(msg_id, &answer, sizeof(answer)-sizeof(long),0);
}

int rejectInvite(int shr_pos, struct studente_t *studente, msginvite_t *rec) {
    msginvite_t answer;

    answer.mtype=rec->mitt;
    answer.mitt=studente->matricola;
    answer.aim='R';
    
    #ifdef DEBUG
    if(project_data->vec[shr_pos].stato_s=='F') {
        printf("[Studente %d - lab %c - i:%d] (%d) rifiuto invito di %ld (motivo NON MI PIACE)\n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE, rec->mitt);
    
    }
    else {
        printf("[Studente %d - lab %c - i:%d] (%d) rifiuto invito di %ld (motivo SONO IN ALTRO GRUPPO)\n", studente->matricola, studente->glab, shr_pos, studente->voto_AdE, rec->mitt);
    }
    #endif    
                    
    return msgsnd(msg_id, &answer, sizeof(answer)-sizeof(long),0);
}