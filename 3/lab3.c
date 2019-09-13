#include<stdio.h>
#include<pthread.h>

#define THREADS_NUM 4
#define words_num 2

struct Param {
	int mes_count;
	char** messages;
};


void* execute_thread(void* args) {

	struct Param param = *(struct Param*)args;
	int i;
	for(i = 0; i < param.mes_count; i++) {
		printf("%s", param.messages[i]);
	}

	return 0;
}

int main(int argc, char* argv[]) {

	pthread_t thread[THREADS_NUM];
   	int status;
    	int status_addr, i, j; 
	char str[] = "Thread says: ";

	 char*** hello;
	 hello = (char***)malloc(sizeof(char**)*THREADS_NUM);
	 struct Param* param;
	 param = (struct Param*)malloc(sizeof(struct Param)*THREADS_NUM);


	for(i = 0; i < THREADS_NUM; i++) {
		hello[i] = (char**)malloc(sizeof(char)*words_num);
		for(j = 0; j < words_num; j++) {
			(hello[i])[j] = (char*)malloc(sizeof(char)*256);
			int n = sprintf((hello[i])[j], "%s hello from thread %d\n", str, i);
		}

		param[i].mes_count = words_num;
		param[i].messages = hello[i];

    		status = pthread_create(&thread[i], NULL, execute_thread, &(param[i]));
		if(status != 0)
			fprintf(stderr, "Error creating thread\n");
	}

	for(i = 0; i < THREADS_NUM; i++) {
		status = pthread_join(thread[i], (void**)&status_addr);
		if(status != 0)
			fprintf(stderr, "Error joining thread\n");
	}

	for(i = 0; i < THREADS_NUM; i++) {
		for(j = 0; j < words_num; j++)
			free(hello[i][j]);
		free(hello[i]);
	}
	free(hello);
	free(param);
	return 0;
}
