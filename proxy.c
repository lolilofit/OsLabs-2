#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include <netdb.h>
#define MAX_HEADER_SIZE 2048


void parse_request(char* request, char** response) {
        int i = 0;
        char del[]= " ";
        char* ptr = strtok(request, del);
        while(ptr != NULL){
                printf("%s\n", ptr);
                strcpy(response[i], ptr);
                i++;
                ptr = strtok(request, del);
        }
        //if(i < 2)
        char head[] = ":/\/";
        ptr = strtok(response[1], head);
        ptr = strtok(response[1], NULL);
        strcpy(response[1], ptr);
}



int get_remote_socket(char* host, char* port) {
        int remote_port = 80, sc;
        struct sockaddr_in sc_addr;

        if(strlen(port) > 0)
                remote_port = atoi(port);
        struct hostent *hp;
        hp = gethostbyname(host);

        if(hp == NULL) {
                printf("wrong url\n");
                return -1;
        }
//      struct in_addr* adr = ((struct in_addr**)(hp -> h_addr_list))[0];
        sc = socket(AF_INET, SOCK_STREAM, 0);
        sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = ((struct in_addr*)((hp->h_addr_list)[0]))->s_addr;
        sc_addr.sin_port = htons(remote_port);
        bind(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr));
        return sc;
}

void split_url(char* url, char* host, char* path) {
        char *pos = strchr(url, '/');
        strncpy(host, host, pos-host);
        strcpy(path, pos);
        printf("host: %s\n", host);
        printf("path: %s\n", path);
}

void form_http_request(char** arg, char* host, char* path) {
        char header[MAX_HEADER_SIZE] = "";
        strcpy(header, arg[0]);
        strcpy(header, " ");
        strcpy(header, path);
        strcpy(header, " ");
        strcpy(header, arg[2]);
        strcpy(header, "\r\nHost: ");
        strcpy(header, host);
        strcpy(header, "\r\nConnection : close\r\n");
        printf("header: %s\n", header);
}

int main(int argc, char* argv[]) {
        int i, sc, conn;
        struct sockaddr_in sc_addr;
        char buf[2048];
        int addrlen = sizeof(sc_addr);

        sc = socket(AF_INET, SOCK_STREAM, 0);
        if(sc == -1) {
                printf("can't create socket\n");
                return 1;
        }

//      int opt = 1;
//      if (setsockopt(sc, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//              perror("setsockopt");
//              exit(-1);
//      }


        sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sc_addr.sin_port = htons(5000);
        if(bind(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr)) < 0) {
                printf("can't bind");
                return -1;
        }

        printf("binded: %d\n", sc);
        if(listen(sc, SOMAXCONN) < 0) {
                printf("errir listen\n");
                return -1;
        }

        //---
        if((conn = accept(sc, (struct sockaddr*)&sc_addr, (socklen_t*)&addrlen)) < 0) {
                printf("error accept\n");
                return -1;
        }
        printf("accepted\n");
        //read(sc, buf, 2048);
        int readen = recv(sc, buf, 2048, 0);
        if(readen < 0)
        printf("error reading\n");
        //printf("readen: %s\n", buf);
        char** response;
        response = (char**)malloc(sizeof(char*)*4);
        for(i=0; i<4; i++) {
                response[i] = (char*)malloc(sizeof(char)*2048);
                strcpy(response[i], "\0");
        }
        parse_request(buf, response);
        char host[2048];
        char path[2048];
        char head[2048];
        split_url(response[1], host, path);
        form_http_request(response, host, path);
        int remote = get_remote_socket(host, response[3]);
        if(remote == -1) {
                printf("bad\n");
        }
        int res = write(remote, head, strlen(head));
        read(remote, buf, 2048);
        printf("%s\n", buf);

        for(i = 0; i < 4; i++)
                free(response[i]);
        free(response);

        //close(sc);
        return 0;
}
