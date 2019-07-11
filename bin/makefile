#Makefile per il Progetto Sistemi Operativi

all: programManager corsoMaster studente debugfile

debug: programManager.debug corsoMaster.debug studente.debug tdebug

#Questi sono i target per la compilazione senza debug
programManager: ../src/header.h ../src/programManager.c debugfile
	gcc  -o programManager ../src/programManager.c

corsoMaster: ../src/header.h ../src/corsoMaster.c debugfile
	gcc  -o corsoMaster ../src/corsoMaster.c

studente: ../src/header.h ../src/studente.c debugfile
	gcc  -o studente ../src/studente.c

#Questi sono i target per la compilazione con debug
programManager.debug: ../src/header.h ../src/programManager.c
	gcc -DDEBUG -o programManager ../src/programManager.c

corsoMaster.debug: ../src/header.h ../src/corsoMaster.c
	gcc -DDEBUG -o corsoMaster ../src/corsoMaster.c

studente.debug: ../src/header.h ../src/studente.c
	gcc -DDEBUG -o studente ../src/studente.c


tdebug: 
	sleep 1
	touch debugfile

#Target per cancellare gli eseguibili
clean: 
	rm -f studente corsoMaster programManager
	touch debugfile


