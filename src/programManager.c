#include "header.h"

void showMenu();
void showGuide();

int main () {
    showMenu();
}

void showMenu(){
    char input;
    system("clear");
    printf("------ Benvenuto su Corso di Sistemi Operativi! ------\n\n"
                   "-Premi g per generare i gruppi e visualizzare i voti\n"
                   "-Premi ? per visualizzare guida & credits\n"
                   "-Premi q per uscire\n\n");
    printf("Scelta: ");
    scanf(" %c", &input);

    switch(input){
        case 'g':
            execve("./corsoMaster",(char *[]){ "./corsoMaster ", NULL, NULL }, NULL);
            
        case '?':
            showGuide();
            break;
        case 'q':
            exit(0);
        default:break;
    }

}



void showGuide() {
    char input;
    system("clear");
    //TODO printf info
    printf("\n\nPremi un tasto e invio per tornare alla schermata iniziale\n");
    scanf(" %c",&input);
    showMenu();
}