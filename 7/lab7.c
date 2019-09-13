#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>


#define ITERATIONS_NUM 500
int THREADS_NUM = 0;

struct Param {
	int offset;
	double result;
};


void* execute_thread(void* args) {
	int i;
	double pi = 0.0;
	struct Param* param = (struct Param*)args;
	
	for(i = param->offset; i < ITERATIONS_NUM; i+=THREADS_NUM) {
		pi += 1.0/(i*4.0 + 1.0);
         	pi -= 1.0/(i*4.0 + 3.0);
	}
	printf("thread-%d result=%f\n", param->offset, pi);
	((struct Param*)args)->result = pi;

      return args;
}

int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("bad arguments number");
		return 0;
	}
	THREADS_NUM = atoi(argv[1]);

        pthread_t thread[THREADS_NUM];
        int status;
        int status_addr, i, j;

	struct Param* params;
	params = (struct Param*)malloc(sizeof(struct Param)*THREADS_NUM);

        for(i = 0; i < THREADS_NUM; i++) {
        	params[i].offset = i;
	        status = pthread_create(&thread[i], NULL, execute_thread, &(params[i]));
           	if(status != 0)
                        fprintf(stderr, "Error creating thread\n");

        }

	double result = 0.0;
        for(i = 0; i < THREADS_NUM; i++) {
		struct Param* res;
                status = pthread_join(thread[i], (void**)&res);
		result += res->result;
	 }

	result = result * 4.0;
    	printf("pi done - %.15g \n", result);
        return 0;
}

