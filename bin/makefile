#Makefile per il Progetto Sistemi Operativi

#OCCHIO SOSTITUIRE cane con studente!!!

#effettua link se uno dei compilati dovesse cambiare
#programManager: programManager.o corsoMaster.o  cane.o 
#	gcc -o programManager programManager.o corsoMaster.o  cane.o

all: programManager corsoMaster studente

programManager: header.h programManager.c
	gcc -o programManager programManager.c

corsoMaster: header.h corsoMaster.c
	gcc -o corsoMaster corsoMaster.c

studente: header.h studente.c
	gcc -o studente studente.c

clean: 
	rm -f *.o studente corsoMaster programManager


