#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<termios.h>
#include <sys/stat.h>
#include <fcntl.h>



//#define THREADS_NUM 3
//#define ITERATIONS_NUM 10
#define ITERATIONS_TO_CHECK 10000
int THREADS_NUM = 0;
int flag = 0;


void quit_handler(int sign) {
	signal(sign, SIG_IGN);
	if(sign == SIGINT) {
		printf("flag is true\n");		
		flag = 1;		
	}
}


struct Param {
        int offset;
        double result;
};


void* execute_thread(void* args) {
	struct sigaction action;
 
        action.sa_flags = SA_SIGINFO; 
        action.sa_sigaction = quit_handler;
 
        if (sigaction(SIGINT, &action, NULL) == -1) { 
            perror("sigusr: sigaction");
            _exit(1);
        }
  	
	double pi = 0.0;
        struct Param* param = (struct Param*)args;
	int i = param->offset;

	while(1) {
       // for(i = param->offset; i < ITERATIONS_NUM; i+=THREADS_NUM) {
                if((i/THREADS_NUM)%ITERATIONS_TO_CHECK == 0) {
		   if(flag == 1) {
			printf("thread-%d result=%f\n", param->offset, pi);
		        ((struct Param*)args)->result = pi;
			pthread_exit(args);
			break;
		   }
		}
		pi += 1.0/(i*4.0 + 1.0);
                pi -= 1.0/(i*4.0 + 3.0);
        	i += THREADS_NUM;
	}

      return 0;
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


        //struct Param params[THREADS_NUM];
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
	free(params);
        return 0;
}

