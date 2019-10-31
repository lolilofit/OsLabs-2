#include<stdio.h>
#include<pthread.h>
#include<malloc.h>
#include<stdlib.h>
#include<string.h>

#define THREADS_NUM 4
#define words_num 2

struct Param {
	int mes_count;
	int offset;
	char** messages;
};


void* execute_thread(void* args) {
	int i;
	struct Param param = *((struct Param*)args);

	for(i = 0; i < param.mes_count; i++) {
		printf("%s\n", param.messages[param.offset + i]);
	}

	return 0;
}

int main(int argc, char* argv[]) {

	pthread_t thread[THREADS_NUM];
   	int status;
    	int status_addr, i, j; 
	struct Param* param;
	param = (struct Param*)malloc(sizeof(struct Param)*THREADS_NUM);

	//char message[THREADS_NUM*words_num+1][256];
	char** message;
	message = (char**)malloc(sizeof(char*)*THREADS_NUM*words_num);
	for(i = 0; i < THREADS_NUM*words_num; i++)
		message[i] = (char*)malloc(sizeof(char)*256);

	for(i = 0; i < THREADS_NUM*words_num; i++) {
			sprintf(message[i], "hello from thread %d-%d", i/words_num, i);
	}

	for(i = 0; i < THREADS_NUM; i++) {

		param[i].mes_count = words_num;
		param[i].offset = i*words_num;
		param[i].messages = message;


    		status = pthread_create(&thread[i], NULL, execute_thread, &(param[i]));
		if(status != 0)
			fprintf(stderr, "Error creating thread\n");
	}

	for(i = 0; i < THREADS_NUM; i++) {
		status = pthread_join(thread[i], (void**)&status_addr);
		if(status != 0)
			fprintf(stderr, "Error joining thread\n");
	}

	for(i = 0; i< THREADS_NUM*words_num; i++)
		free(message[i]);
	free(message);
	free(param);
	return 0;
}
