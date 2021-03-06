
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>

struct List{
        struct List* next;
        char* str;
	pthread_mutex_t list_mutext;
};

void free_mem(struct List* list) {
        if(list->next == NULL) {
                free(list->str);
        }
        else {
                free_mem(list->next);
                free(list);
        }
}

struct List* create_head() {
        struct List* head;
        head = (struct List*)malloc(sizeof(struct List));
        head->next = NULL;
        head->str = NULL;
	pthread_mutex_init(&(head->list_mutext), NULL);
        return head;
}

void add(struct List* list, char* str){
	struct List* el = (struct List*)malloc(sizeof(struct List));
        el->str = str;
        pthread_mutex_init(&(el->list_mutext), NULL);

//	printf("add mutext try lock\n");
	pthread_mutex_lock(&(list->list_mutext));
//	printf("add mutext locked\n");
        struct List* tmp = list->next;
        el->next = tmp;
        list->next=el;
//	printf("add mutext try unlock\n");
	pthread_mutex_unlock(&(list->list_mutext));
//	printf("add mutext unlock\n");
}


void print_all(struct List* list) {
//              printf("print mutext list try lock\n");
                struct List* prev = list;
		pthread_mutex_lock(&(prev->list_mutext));
//              printf("print mutext list lock\n");


        if(list != NULL) {
		if(list->next != NULL) {
                struct List* cur = list->next;
                printf("\n");
//		 pthread_mutex_lock(&(cur->list_mutext));
                while(cur) {
//			printf("print mutext cur try lock\n");
			pthread_mutex_lock(&(cur->list_mutext));
			 pthread_mutex_unlock(&(prev->list_mutext));
//			printf("print mutext cur lock\n");
//          		pthread_mutex_unlock(&(list->next->list_mutext));
			printf("%s\n", cur->str);
//			 printf("print mutext cur try unlock\n");
//			pthread_mutex_unlock(&(cur->list_mutext));
//			 printf("print mutext cur  unlock\n");
			cur = cur->next;
                }
		}
//		 pthread_mutex_unlock(&(cur->list_mutext));
	}
//		 printf("print mutext list try unlock\n");
		pthread_mutex_unlock(&(rev->list_mutext));
//		 printf("print mutext list unlock\n");
//                printf("\n");

}

void swap(struct List* first, struct List* second) {
        char* tmp = first->str;
        first->str = second->str;
        second->str = tmp;
}

void* sort(void* args) {
        while(1) {
                struct List* list = ((struct List*)args);
//		printf("sort list mutext try lock\n");
		pthread_mutex_lock(&(list->list_mutext));
//		 printf("sort list mutext lock\n");
		struct List* cur = list->next;
		struct List* inner_cur;

		 pthread_mutex_lock(&(cur->list_mutext));
                while(cur) {
//			printf("sort cur mutext try lock\n");
//			pthread_mutex_lock(&(cur->list_mutext));
//			printf("sort cur mutext  lock\n");
			inner_cur = cur->next;

                        while(inner_cur) {
//				 printf("sort inner cur mutext try lock\n");
				pthread_mutex_lock(&(inner_cur->list_mutext));
//				 printf("sort inner cur mutext lock\n");
                                if(strcmp(cur->str, inner_cur->str) > 0)
                                        swap(inner_cur, cur);
//				 printf("sort inner cur mutext try unlock\n");
				 pthread_mutex_unlock(&(inner_cur->list_mutext));
//				 printf("sort inner cur mutext unlock\n");
				 inner_cur = inner_cur->next;
                        }
//			 printf("sort cur mutext try unlock\n");
			//pthread_mutex_unlock(&(cur->list_mutext));
//			 printf("sort cur mutext unlock\n");
                	cur = cur->next;
		}
		 pthread_mutex_unlock(&(cur->list_mutext));

//		 printf("sort list mutext try unlock\n");
		pthread_mutex_unlock(&(list->list_mutext));
//		 printf("sort list mutext unlock\n-------------\n");

                sleep(5);
        }
}

int main(int argc, char* argv[]){
        pthread_t thread;
        int status;
        int status_addr;
        char* get = NULL;
        struct List* list;
        list = create_head();

        status = pthread_create(&thread, NULL, sort, (void*)list);

        while(1) {
                get = (char*)malloc(sizeof(char)*81);
                fgets(get, 80, stdin);
                printf("got : %s\n", get);

                if(strlen(get) == 1) {
                        print_all(list);
			continue;
                }
                	add(list, get);
		
        }

        pthread_cancel(thread);
        pthread_join(thread, NULL);
	struct List* cur = list;
	while(cur) {
		 pthread_mutex_destroy(&(cur->list_mutext));
	}
        return 0;
}

