#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/types.h>
#include<termios.h>
#include <sys/stat.h>
#include <fcntl.h>


#define ITERATIONS_TO_CHECK 10000
int THREADS_NUM = 0;
int flag = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;
int max_iter = 0;

void quit_handler(int sign) {
	signal(sign, SIG_IGN);
	if(sign == SIGINT) {
		printf("flag is true\n");		
		pthread_mutex_lock(&mutex);
		flag = 1;		
		pthread_mutex_unlock(&mutex);
	}
}


struct Param {
        int offset;
        double result;
};


void* execute_thread(void* args) {
	struct sigaction action;
 	struct sigaction ign;

        action.sa_flags = SA_SIGINFO; 
        action.sa_sigaction = quit_handler;
	ign.sa_flags = SA_SIGINFO;
        ign.sa_sigaction = SIG_IGN;
 
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

		 pthread_mutex_lock(&mutex);
		   if(i > max_iter)
			max_iter = i;
		   if((flag == 1) && (i==max_iter)) {
			pthread_mutex_unlock(&mutex);
			printf("thread-%d result=%f\n", param->offset, pi);
		        ((struct Param*)args)->result = pi;
			pthread_exit(args);
			break;
		   }
	/*
		if(flag != 1) {
		   if (sigaction(SIGINT, &ign, NULL) == -1) {
                    perror("sigusr: sigaction");
                    _exit(1);
                   }
                // pthread_mutex_lock(&mutex);
                //        pthread_mutex_unlock(&mutex);
                        pthread_barrier_wait(&barrier);

                   if (sigaction(SIGINT, &action, NULL) == -1) {
                    perror("sigusr: sigaction");
                    _exit(1);
                   }
		}
	*/
		  pthread_mutex_unlock(&mutex);
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
	pthread_barrier_init(&barrier, NULL, THREADS_NUM);

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
	pthread_barrier_destroy(&barrier);
	free(params);
        return 0;
}

