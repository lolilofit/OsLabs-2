#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include"elements.h"

int main(int argc, char* argv[]) {
	int sem, i;
	struct sembuf detail;

	setbuf(stdout,NULL);

	if(!strcmp("a", argv[1]))
		detail.sem_num = A_NUM;
	if(!strcmp("b", argv[1]))
                 detail.sem_num = B_NUM;
	if(!strcmp("c", argv[1]))
	         detail.sem_num = C_NUM;
	
	detail.sem_op = 1; 
	detail.sem_flg = SEM_UNDO;

	int timing = atoi(argv[2]);
	sem = atoi(argv[3]);

	unsigned short buff[4];
//	for(i = 0; i < VIDGETS_NUM; i++) {
	while(1) {
		sleep(timing);
		printf("Created detail %s\n", argv[1]);
		semop(sem, &detail, 1);
	}


	return 0;
}
