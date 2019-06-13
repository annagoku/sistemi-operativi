#Makefile per il Progetto Sistemi Operativi

all: programManager corsoMaster studente

programManager: ../src/header.h ../src/programManager.c
	gcc -o programManager ../src/programManager.c

corsoMaster: ../src/header.h ../src/corsoMaster.c
	gcc -o corsoMaster ../src/corsoMaster.c

studente: ../src/header.h ../src/studente.c
	gcc -o studente ../src/studente.c

clean: 
	rm -f *.o studente corsoMaster programManager


