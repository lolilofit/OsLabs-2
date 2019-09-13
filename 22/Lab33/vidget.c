#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <signal.h>
#include"elements.h"

int sem;

void end() {
	 if(semctl(sem, 0 ,IPC_RMID, 0) == -1) {
                perror("semctl");
                exit(5);
        }
	kill(0, SIGTERM);
	exit(0);
}

enum timings {a = 2, b, c};

int main(int argc, char* argv[]) {

	static struct sembuf op[2] = {{C_NUM, -1, SEM_UNDO}, {MODULS_NUM, -1, SEM_UNDO}};

	
	if((sem = semget(IPC_PRIVATE, 4, IPC_CREAT | 0640))==-1)  {
        	perror(argv[0]);
        	exit(1);
  	  }
	
	sigset(SIGINT, end);
	char sem_str[256];
	sprintf(sem_str, "%d", sem);
	char line[256];


	if(fork() == 0) {
		sprintf(line, "%d", a);
		execlp("./detail", "detail", "a", line, sem_str, (char*)(0));
		perror("A forking fail");
		exit(2);
	}
	 if(fork() == 0) {
		sprintf(line, "%d", b);
                execlp("./detail", "detail", "b", line, sem_str, (char*)(0));
                perror("B forking fail");
                exit(2);
        }
	if(fork() == 0) {
		sprintf(line, "%d", c);
                execlp("./detail", "detail", "c", line, sem_str, (char*)(0));
                perror("C forking fail");
                exit(2);
        }
	if(fork() == 0) {
                execlp("./modul", "modul", sem_str, (char*)(0));
                perror("Module forking fail");
                exit(2);
        }

	unsigned short buff[4];
	
//	for(i = 0; i < VIDGETS_NUM; i++) {
	int count = 0;
	while(1) {
		if(semop(sem, op, 2) == -1) {
			perror(argv[0]);
			exit(3);
		}
		count++;
		printf("Do vidget with number=%d\n", count);
	}

	return 0;
}
