#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include"elements.h"

int main(int argc, char* argv[]) {
	int sem;
	static struct sembuf modul_details[2] = {{A_NUM, -1, SEM_UNDO}, {B_NUM, -1, SEM_UNDO}};
	static struct sembuf modul = {MODULS_NUM, 1, SEM_UNDO};
	sem = atoi(argv[1]);
	
	int i;
	unsigned short buff[4];

//	setbuf(stdout,NULL);
 	//for(i = 0; i < VIDGETS_NUM; i++) {
	int count = 0;
	while(1) {
		if(semop(sem, &modul_details[0], 2) == -1) {
			perror(argv[0]);
			exit(1);
		}
		count++;
		printf("Crate a modul with number - %d\n", count);
	
		if (semop(sem, &modul, 1) == -1) {
			perror(argv[0]);
                        exit(1);
		}
	}
	
	return 0;
}
