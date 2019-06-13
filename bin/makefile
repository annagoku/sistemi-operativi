#Makefile per il Progetto Sistemi Operativi

#OCCHIO SOSTITUIRE cane con studente!!!

#effettua link se uno dei compilati dovesse cambiare
#programManager: programManager.o corsoMaster.o  cane.o 
#	gcc -o programManager programManager.o corsoMaster.o  cane.o

all: programManager corsoMaster studente

programManager: ../src/header.h ../src/programManager.c
	gcc -o programManager ../src/programManager.c

corsoMaster: ../src/header.h ../src/corsoMaster.c
	gcc -o corsoMaster ../src/corsoMaster.c

studente: ../src/header.h ../src/studente.c
	gcc -o studente ../src/studente.c

clean: 
	rm -f *.o studente corsoMaster programManager


