
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
struct List* list;

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

	pthread_mutex_lock(&(list->list_mutext));
        struct List* tmp = list->next;
        el->next = tmp;
        list->next=el;
	pthread_mutex_unlock(&(list->list_mutext));
}


void print_all(struct List* list) {
	if(list != NULL) {
		struct List* prev = list;  
        	pthread_mutex_lock(&(prev->list_mutext));

		//if(list->next != NULL) {
                struct List* cur = prev->next;
                printf("\n");
                while(cur) {
			pthread_mutex_lock(&(cur->list_mutext));
			pthread_mutex_unlock(&(prev->list_mutext));
			printf("%s\n", cur->str);
			//pthread_mutex_unlock(&(cur->list_mutext));
			prev = cur;
			cur = cur->next;
		}
		 pthread_mutex_unlock(&(prev->list_mutext));
	//	}
	}
}

void swap(struct List* first, struct List* second) {
        char* tmp = first->str;
        first->str = second->str;
        second->str = tmp;
}

void* sort(void* args) {
	int end;
	int time_to_sleep = (int)args;

        while(1) {
	//	sleep(time_to_sleep);	
		end = 1;
		while(end) {
                	//struct List* prev = ((struct List*)args);
			struct List* prev = list;
			pthread_mutex_lock(&(prev->list_mutext));
			struct List* cur = prev->next;
			struct List* inner_cur;
			end=0;
			if(cur != NULL) {
				pthread_mutex_lock(&(cur->list_mutext));
				inner_cur = cur->next;
				while(inner_cur) {
					pthread_mutex_lock(&(inner_cur->list_mutext));
					if(strcmp(inner_cur->str, cur->str) < 0) {
						sleep(1);
						cur->next=inner_cur->next;
						prev->next = inner_cur;
						inner_cur->next = cur;
						cur=inner_cur;
						inner_cur=cur->next;
						end = 1;
					}
					pthread_mutex_unlock(&(prev->list_mutext));
					prev=cur;
					cur=inner_cur;
					inner_cur=inner_cur->next;
				}
				pthread_mutex_unlock(&(cur->list_mutext));
			}
			pthread_mutex_unlock(&(prev->list_mutext));
		}

        }
}

int main(int argc, char* argv[]){
        pthread_t thread;
        int status;
        int status_addr;
        char* get = NULL;
        list = create_head();
	
	int sort_num = 1, i;
	if(argc == 2)
		sort_num = atoi(argv[1]);

	for(i = 0; i < sort_num; i++)
        	status = pthread_create(&thread, NULL, sort, (void*)(5+i));

        while(1) {
                get = (char*)malloc(sizeof(char)*81);
                fgets(get, 80, stdin);
                printf("got : %s\n", get);

                if(strlen(get) == 1) {
                        print_all(list);
			continue;
                }
		else {
                	add(list, get);
		}
        }

        pthread_cancel(thread);
        pthread_join(thread, NULL);
	struct List* cur = list;
	while(cur) {
		 pthread_mutex_destroy(&(cur->list_mutext));
	}
        return 0;
}

