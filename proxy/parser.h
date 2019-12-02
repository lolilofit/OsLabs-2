#ifndef PARSER_H
#define PARSER_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include <fcntl.h>
#include <sys/file.h>
#include"definings.h"
#include "list.h"

struct HttpParams {
        char path[MAX_HEADER_SIZE+1];
        char host[MAX_HEADER_SIZE+1];
        char method[30];
        char protocol[30];
};

struct HttpAnswer {
        char status[4];
};

void pars_path(struct HttpParams* response) {
        char* tmp;
        tmp  = strstr(response->path, response->host);
        if(tmp != NULL) {
                char *end = tmp  + strlen(response->host);
                strcpy(response->path, end);
        }
}

void pars_line(char* line, struct HttpParams* response, int is_first) {
        char* ptr;
        int flag = 0, i = 0;

        ptr = strtok(line, " ");

        if(is_first == 1)
                strcpy(response->method, ptr);

        while(ptr != NULL) {
                if(flag == 1)
                        strcpy(response->host, ptr);
                if(strcmp(ptr, "Host:") == 0)
                        flag = 1;

                if(i == 1 && is_first == 1)
                         strcpy(response->path, ptr);
                if(i == 2 && is_first == 1)
                        strcpy(response->protocol, ptr);
                i++;
                ptr = strtok(NULL, " ");
        }
}


void pars_answer_line(char* line,  struct HttpAnswer* remote_answer, int is_first) {
        char* ptr;
        int flag = 0, i = 0;

        ptr = strtok(line, " ");
        while(ptr != NULL) {
                if(i == 1 && is_first == 1)
                         strcpy(remote_answer->status, ptr);
                i++;
                ptr = strtok(NULL, " ");
        }
}

void parse_request(char* request, struct HttpParams* response, struct HttpAnswer* remote_answer) {
        int i = 0, flag = 0;
        char* ptr;
        char tmp[MAX_HEADER_SIZE+1];

        struct List* head;
        head = (struct List*)malloc(sizeof(struct List));
        head->next = NULL;

        struct List* cur = head;

        ptr = strtok(request, "\r\n");

        while(ptr != NULL) {
                struct List* el;
                el = (struct List*)malloc(sizeof(struct List));
                strcpy(el->str, ptr);
                el->next = NULL;
                cur->next = el;
                cur = el;
                if(remote_answer != NULL)
                        ptr = NULL;
                else
                        ptr = strtok(NULL, "\r\n");
        }

        head = head->next;
        while(head != NULL) {
                if(flag == 0) {
                        if(response != NULL)
                                pars_line(head->str, response, 1);
                        if(remote_answer != NULL)
                                pars_answer_line(head->str, remote_answer, 1);
                        flag = 1;
                }
                else {
                        if(response != NULL)
                                pars_line(head->str, response, 0);
                }
                cur = head;
                head = head->next;
                free(cur);
        }
	 if(response != NULL) {
                pars_path(response);
                printf("path is %s\n", response->path);
        }
}




#endif
