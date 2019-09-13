#include<stdio.h>
#include<pthread.h>


void* do_work(void *args) {
     
        int i;
        for(i = 0; i < 10; i++) {
                printf("thread 1 - %d\n", i);
        }
        return 0;
}


int main(int argc, char** argv) {
        pthread_t thread;
        int status;
        int status_addr;

        status = pthread_create(&thread, NULL, do_work, NULL);

        if(status != 0) {
                fprintf(stderr, "Error creating thread\n");
        }

        status = pthread_join(thread, (void*)&status_addr);
        if(status != 0) {
                fprintf(stderr, "error\n");
        }

        int i;
        for(i = 0; i < 10; i++) {
                printf("thread 0 - %d\n", i);
        }


        return 0;
}

