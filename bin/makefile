#Makefile per il Progetto Sistemi Operativi

all: programManager.nodebug corsoMaster.nodebug studente.nodebug

debug: programManager.debug corsoMaster.debug studente.debug

#Questi sono i target per la compilazione senza debug
programManager.nodebug: ../src/header.h ../src/programManager.c
	gcc  -o programManager ../src/programManager.c

corsoMaster.nodebug: ../src/header.h ../src/corsoMaster.c
	gcc  -o corsoMaster ../src/corsoMaster.c

studente.nodebug: ../src/header.h ../src/studente.c
	gcc  -o studente ../src/studente.c


#Questi sono i target per la compilazione con debug
programManager.debug: ../src/header.h ../src/programManager.c
	gcc -DDEBUG -o programManager ../src/programManager.c

corsoMaster.debug: ../src/header.h ../src/corsoMaster.c
	gcc -DDEBUG -o corsoMaster ../src/corsoMaster.c

studente.debug: ../src/header.h ../src/studente.c
	gcc -DDEBUG -o studente ../src/studente.c

#Target per cancellare gli eseguibili
clean: 
	rm -f studente corsoMaster programManager


