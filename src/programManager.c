#include "header.h"

void showMenu();
void showGuide();

int main () {
    system("clear");
    showMenu();
}

void showMenu(){
    char input;
    
    printf("------ Benvenuto su Corso di Sistemi Operativi! ------\n\n"
                   "-Premi g per generare i gruppi e visualizzare i voti\n"
                   "-Premi ? per visualizzare guida & credits\n"
                   "-Premi q per uscire\n\n");
    printf("Scelta: ");
    scanf(" %c", &input);

    input = tolower(input);

    switch(input){
        case 'g':
            execve("./corsoMaster",(char *[]){ "./corsoMaster ", NULL, NULL }, NULL);
            
        case '?':
            system("clear");
            showGuide();

            break;
        case 'q':
            exit(0);
        default:
        system("clear");
        printf("Selezione non consentita. Immettere uno tra i caratteri ammessi\n");
        printf("\n");
        showMenu();
        break;
    }
}



void showGuide() {
    char input;
    printf("-------GUIDE & CREDITS-------\n\n");
    printf("Il programma simula lo scambio di messaggi tra studenti del Corso di Sistemi Operativi\ncon l'obiettivo di formare gruppi di studio coerentemente ad una serie di vincoli e regole\ndefiniti dalla consegna.\nAl termine di un sim time preliminarmente stabilito, ad ogni studente viene assegnato un voto\nper l'esame di Sistemi Operativi che dipenderà dalla coerenza tra il gruppo finale di appartenenza e\nle proprie preferenze iniziali e il voto conseguito nell'esame di Architettura di Elaboratori.\n");
    printf("\n");
    printf("Author: Annalisa Sabatelli\tMatricola: 866879\n\nAnno Accademico: 2018-2019" );
    printf("\n\nPremi un tasto e invio per tornare alla schermata iniziale\n");
    scanf(" %c",&input);
    showMenu();
}