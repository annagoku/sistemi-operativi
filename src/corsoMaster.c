#include "header.h"

int     letturaRighe();
void    letturaFile(gruppo_t *gruppi);
void    suddividiGruppi(char **sudd, gruppo_t *gruppi, int nGruppi);
void    calcoloVotiFinali();
int     conta_membrigruppo (int nome_g);
int     trova_maxvoto (int nome_g);
int     trova_leader (int name_g);
void    numstudentipervotoAdE();
void    numstudentipervotoSO();


pid_t   children[POP_SIZE];
msgFromMaster_t votiFinali[POP_SIZE];

int sem_init; // identificatore del vettore di semafori
int shr_mem; //identificatore della memoria condivisa
int msg_id; //identificatore della coda di messaggi
int msg_master;//identificatore della coda di messaggi master finale
char nof_invites[3];

struct shared_data *project_data;
struct sembuf shr;

void reset_sem();
void fill_semshr();
void handle_alarm (int signal);

int main(int argc, char **argcv) {
    char buffer[2];
    
    char* suddivisioneGruppi[POP_SIZE];
    pid_t pid, child;
    int n = POP_SIZE;
    int status;
    int i = 0;
    float check=0;
    char str[2];
    gruppo_t *gruppi;
    int nGruppi;
    struct sembuf presenza; //struttura dati di sistema per i semafori 
    char printMedia;
    float mediaAdE=0;
    int sommaAdE=0;
    float mediaSO=0;
    int sommaSO=0;
    
    struct sigaction end;  // strutture dati di sistema per la gestione dell'alarm
    sigset_t  mymask;

   
    nGruppi = letturaRighe();
    
    gruppi = (gruppo_t*) malloc(i*sizeof(gruppo_t));

    letturaFile(gruppi);
    

    //printf("sizeof(gruppi)  %d \n", nGruppi);

    for(i=0; i< nGruppi; i++) {
        //printf("Gruppo  %d - prob %f\n", gruppi[i].nof_elems, gruppi[i].prob);
        check = check + gruppi[i].prob;
    }
    if(check > 1) {
        //printf("La percentuale dei gruppi non è corretta\n");
        exit(EXIT_FAILURE);
    }
    suddividiGruppi(suddivisioneGruppi, gruppi, nGruppi);
    /*for(i=0; i<POP_SIZE; i++) {
        printf("Studente  %d - gruppo %s\n", i, suddivisioneGruppi[i]);
    }*/
    printf("[CorsoMaster] Step 0 - Lettura file OK, Suddivisione Gruppi OK\n");

    printf("[CorsoMaster]          Studenti totali %d - #tipologia gruppi %d - max rifiuti %d - simtime %d\n", POP_SIZE, nGruppi, MAX_REJECT, SIM_TIME);

    // gestione segnale ctrl +c 
    end.sa_handler=handle_alarm;
    end.sa_flags=0;
    sigemptyset(&mymask);
    end.sa_mask=mymask;
    sigaction(SIGINT, &end, NULL);

    //impostazione di un vettore di semafori per sincronizzazione  figli
    if ((sem_init=semget(SEM_ID, 2, IPC_CREAT| 0666))<0){
        printf("[CorsoMaster] Errore nella creazione del vettore di semafori \n ");
        exit(-1);
    }
    printf("[CorsoMaster] Step 1 - creazione vettore di semafori OK\n");
    reset_sem();
    fill_semshr();
    

    //----------------------------impostazione memoria condivisa
    if((shr_mem=shmget(SHR_ID, POP_SIZE*sizeof(info_shr_t), IPC_CREAT|0666))<0 ){
        printf("[CorsoMaster] Errore nella creazione della memoria condivisa \n ");
        perror("shmget");
        exit(-1);
    }
    project_data=shmat(shr_mem, NULL, 0);
    project_data->cur_idx=0;
    
     
    printf("[CorsoMaster] Step 2 - Imposto la coda di messaggi\n");
     
    //----------------------------impostazione coda di messaggi


    if((msg_id=msgget(MSG_ID, IPC_CREAT | IPC_EXCL | 0600))==-1) {
        perror("msgget");
        printf("[CorsoMaster] Errore nella creazione della coda di messaggi \n ");
        printf("[CorsoMaster] errno=%d", errno);
        exit(-1);
    }
    //----------------------------impostazione coda di messaggi master finale


    if((msg_master=msgget(MSG_MASTER, IPC_CREAT | IPC_EXCL | 0600))==-1) {
        perror("msgget");
        printf("[CorsoMaster] Errore nella creazione della coda di messaggi voto finale \n ");
        printf("[CorsoMaster] errno=%d", errno);
        exit(-1);
    }


    printf("[CorsoMaster] Step 3 - Genero i processi figli (studenti)\n");

    i=0;
    while(i < POP_SIZE) {
        child = fork();
        switch (child)
        {
            case -1:
                perror("[CorsoMaster] Errore nel creare i processi figli");
                exit(-1);
                break;
            case 0:
                // child process

                execve("./studente",(char *[]){ "./studente ", suddivisioneGruppi[i], nof_invites }, NULL);
                exit(0);
                break;
        
            default:
                //parent process
                children[i] = child;
                break;
        }
        i++;

    }
    //master in attesa del completamento inizializzazione figli
    presenza.sem_flg=0;
    presenza.sem_num=INIT_READY;
    presenza.sem_op=-POP_SIZE;
    semop(sem_init, &presenza, 1);

    printf("[CorsoMaster] Step 4 ------------------------------- Partenza cronometro\n");
    //sleep(1);
    
    //Mando un segnale per risvegliare gli studenti
    for(i=0; i< POP_SIZE; i++) {
        kill(children[i], SIGUSR1);
    }
    //gestione del segnale di fine alarm
    end.sa_handler=handle_alarm;
    end.sa_flags=0;
    sigemptyset(&mymask);
    end.sa_mask=mymask;
    sigaction(SIGALRM, &end, NULL);
    //mando il segnale di ALARM per chiudere lo scambio
    
    alarm(SIM_TIME); 
    //Preallarme
    sleep(SIM_TIME-(SIM_TIME*25/100));
    printf("[CorsoMaster]  Mando preallarme\n");
    for(i=0; i< POP_SIZE; i++) {
        kill(children[i], SIGUSR2);
    }

    while(n > 0){
        pid = wait(&status);
        /*if ( WIFEXITED(status) ){
            
            printf("Processo %d ha finito %d\n", pid, WEXITSTATUS(status));

        }*/
        n--;
    }
    

    printf("[CorsoMaster] Step 5 - Computo della media\n");
    
    msgctl(msg_id, IPC_RMID, NULL);
    msgctl(msg_master, IPC_RMID, NULL);
    semctl(sem_init, 2, IPC_RMID, 0);



    printf("Vuoi stampare ilnumero studenti per ogni voto e il voto medio dei due esami? [Y/N]");
    scanf("%c", &printMedia);

    switch (toupper(printMedia)){

        case 'Y':
            for (i=0; i<POP_SIZE; i++){
                //calcolo media AdE e SO
                sommaAdE=sommaAdE+project_data->vec[i].voto_AdE;
                sommaSO=sommaSO+votiFinali[i].VotoSO;
            }
            //printf("SOMMA AdE: %d\n  SOMMA SO: %d\n", sommaAdE, sommaSO); 
                
            mediaAdE=(float)sommaAdE/(float)POP_SIZE;
            mediaSO=(float)sommaSO/(float)POP_SIZE;

            printf("Numero studenti per voto nell'esame di Architettura di Elaboratori e voto medio complessivo\n");

            numstudentipervotoAdE ();
            printf("\n");

            printf("Voto medio Architettura degli eleboratori %2.2f\n\n", mediaAdE);

            printf("Numero studenti per voto nell'esame di Sistemi Operativi e voto medio complessivo\n\n");

            numstudentipervotoSO ();
            printf("\n");

            printf("Voto medio Sistemi Operativi %2.2f\n\n",  mediaSO);
            
            printf("Fine della simulazione");
            shmctl(shr_mem,IPC_RMID, NULL);
            exit(0);
            break;
            
        case 'N':
            printf("Fine della simulazione\n");
            shmctl(shr_mem,IPC_RMID, NULL);
            exit(0);
            break;
    }
}

int letturaRighe () {
        int i =0;
        char c;
        FILE *fp = fopen("opt.conf", "r");
       
        if( fp == NULL ) {
            printf("[CorsoMaster] Errore nell'apertura del file di configurazione\n");
            exit(EXIT_FAILURE);
        }
        for (c = getc(fp); c != EOF; c = getc(fp)) 
            if (c == '\n') // Increment count if this character is newline 
                i++; 
        fclose(fp);
        return i-1;

}

void letturaFile (gruppo_t *gruppi) {
        float f;
        int nof_elems;
        char s[6];
        int i =0, j=0;
        char line[50];
       
        FILE *file = fopen("opt.conf", "r");
        while (fgets ( line, 50, file ) != NULL){
            //printf("leggo  riga %i ", j+1);

            if(strncmp (line, "nof_invites", 11) == 0) {

                sscanf(line, "%s %s\n", s, nof_invites);
                //printf(" - nof_invites %d\n", nof_elems);
            
            }
            else {
                sscanf(line, "%s %d %f\n", s, &nof_elems, &f);
                //printf("elementi %d - prob %f\n", nof_elems, f);
            
                gruppi[i].nof_elems = nof_elems;
                gruppi[i].prob = f;
                
                sprintf(gruppi[i].nof_elems_str, "%d", gruppi[i].nof_elems);
                if(i>0) {
                    gruppi[i].min = gruppi[i-1].max;
                }
                else {
                    gruppi[i].min = 0;
                }
                gruppi[i].max = gruppi[i].min + gruppi[i].prob;
                i++; 
            }
            j++;
            
    } 

}

void  suddividiGruppi(char **sudd, gruppo_t *gruppi, int nGruppi) {
    int i=0, j=0;
    int trovato=0;
    
    for(i=0; i<POP_SIZE; i++) {
        
        if(i>(gruppi[j].max *(POP_SIZE-1))) {
            j++;
        }
        sudd[i] = gruppi[j].nof_elems_str;
       
    }
}

void reset_sem(){
    semctl(sem_init, INIT_READY, SETVAL, 0);
    semctl(sem_init, SHR_SCRIPT, SETVAL, 0);
}

void fill_semshr(){
    shr.sem_flg=0;
    shr.sem_num=SHR_SCRIPT;
    shr.sem_op=1;
    semop(sem_init, &shr, 1);
}

void handle_alarm (int signal){
    int i=0;
    if(signal==SIGALRM){
        printf("[CorsoMaster] ------------------------------- Fine Cronometro\n");
    
        for (i=0; i<POP_SIZE; i++){
            #ifdef DEBUG
            printf("[CorsoMaster] inoltro fine alarm a %d\n",children[i]);
            #endif
            kill (children[i], SIGALRM);
        }

        calcoloVotiFinali();     

        
    }
    else if (signal==SIGINT){ //handler ctrl + c
        //causa l'immediata terminazione dei figli
        for (i=0; i<POP_SIZE;i++){
            kill (children[i], SIGKILL);
        }
        //elimina tutte le strutture IPC create
        msgctl(msg_id, IPC_RMID, NULL);
        msgctl(msg_master, IPC_RMID, NULL);
        semctl(sem_init, 0, IPC_RMID, 0);
        shmctl(shr_mem,IPC_RMID, NULL);
        printf("Premuto CTRL+C");
        exit -1;
    }
}


void calcoloVotiFinali() {
    int i=0;
    
    printf("[CorsoMaster] ------------------------------- Calcolo voti finali\n");
    for(i=0; i<POP_SIZE; i++){
        if (project_data->vec[i].tipo_componente =='L'){
            if (project_data->vec[i].stato_g=='C'){
                //il gruppo e' chiuso
                if (conta_membrigruppo(project_data->vec[i].nome_gruppo)==project_data->vec[i].nof_elems){
                    votiFinali[i].VotoSO=trova_maxvoto(project_data->vec[i].nome_gruppo);
                }
                else {
                    votiFinali[i].VotoSO=trova_maxvoto(project_data->vec[i].nome_gruppo)-3;
                }
            }
            else  {
                votiFinali[i].VotoSO=0;
            }
            votiFinali[i].mtype=project_data->vec[i].matricola;
            votiFinali[i].VotoAdE=project_data->vec[i].voto_AdE;
            
            if(msgsnd(msg_master, &votiFinali[i], sizeof(votiFinali[i])-sizeof(long),0)==-1){
                printf("[CorsoMaster]\n Errore nell'invio voto finale (studente %d)",project_data->vec[i].matricola);
            }
        }
        else if (project_data->vec[i].tipo_componente =='F'){
            if (project_data->vec[trova_leader(project_data->vec[i].nome_gruppo)].stato_g=='C'){
                if (conta_membrigruppo(project_data->vec[i].nome_gruppo)==project_data->vec[i].nof_elems){
                    votiFinali[i].VotoSO=trova_maxvoto(project_data->vec[i].nome_gruppo);
                }
                else {
                    votiFinali[i].VotoSO=trova_maxvoto(project_data->vec[i].nome_gruppo)-3;
                }
            }
            else {
                votiFinali[i].VotoSO=0;
            }
            votiFinali[i].mtype=project_data->vec[i].matricola;
            votiFinali[i].VotoAdE=project_data->vec[i].voto_AdE;
            
            if(msgsnd(msg_master, &votiFinali[i], sizeof(votiFinali[i])-sizeof(long),0)==-1){
                printf("[CorsoMaster]\n Errore nell'invio voto finale (studente %d)",project_data->vec[i].matricola);
            }
        }  
        else {
            
            votiFinali[i].VotoSO=0;
            
            votiFinali[i].mtype=project_data->vec[i].matricola;
            votiFinali[i].VotoAdE=project_data->vec[i].voto_AdE;
            
            if(msgsnd(msg_master, &votiFinali[i], sizeof(votiFinali[i])-sizeof(long),0)==-1){
                printf("[CorsoMaster]\n Errore nell'invio voto finale (studente %d)",project_data->vec[i].matricola);
            }
        } 
    }

}



int conta_membrigruppo (int nome_g){
     int i=0;
     int count=0;
     
     for (i=0; i<POP_SIZE; i++){
         if(project_data->vec[i].nome_gruppo==nome_g){
             count++;
         }
     }
     return count;
 }
 

       
 int trova_leader (int name_g){
    int k=0;
    for(k=0;k<POP_SIZE;k++) {
        if(project_data->vec[k].nome_gruppo==name_g && project_data->vec[k].tipo_componente=='L') {
            return k;
        }
    }
    
    return k;
 }


 int trova_maxvoto (int nome_g){
     int i=0;
     int max=0;

     for (i=0; i<POP_SIZE; i++){
         if (project_data->vec[i].nome_gruppo==nome_g){
             if (max< project_data->vec[i].voto_AdE){
                 max=project_data->vec[i].voto_AdE;
             }
         }
     }
     return max;
 }

 void numstudentipervotoAdE(){
     int count=0;
     int k=0;
     int i=0;
    
    printf ("Distribuzione voti Architetture di Elaboratori\n\n");
    printf ("# studenti | voto\n");
    printf ("-----------|-----\n");
        
    //lo faccio partire da 0 nel malaugurato caso in cui uno studente non abbia chiuso il gruppo
     for (k=0; k<=30; k++){
        for(i=0; i<POP_SIZE; i++){
            if (votiFinali[i].VotoAdE==k){
                count++;
            }
        }
        printf ("%10d |%5d\n", count, k);
        
        if(k==0) {
            //allora lo pongo uguale a 14 in modo da far testare il 15 al prossimo ciclo del for
            k=14;

        }
        count=0;
    }
    printf ("-----------|-----\n");
    
 }

void numstudentipervotoSO(){
     int count=0;
     int k=0;
     int i=0;
    
    printf ("Distribuzione voti Sistemi Operativi\n\n");
    printf ("# studenti | voto\n");
    printf ("-----------|-----\n");
        
    //lo faccio partire da 0 nel malaugurato caso in cui uno studente non abbia chiuso il gruppo
     for (k=0; k<=30; k++){
        for(i=0; i<POP_SIZE; i++){
            if (votiFinali[i].VotoSO==k){
                count++;
            }
        }
        printf ("%10d |%5d\n", count, k);
        
        if(k==0) {
            //allora lo pongo uguale a 14 in modo da far testare il 15 al prossimo ciclo del for
            k=14;

        }
        count=0;
    }
    printf ("-----------|-----\n");
    
 }
 