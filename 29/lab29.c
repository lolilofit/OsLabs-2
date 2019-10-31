#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<netdb.h>
#define MAX_HEADER_SIZE 2048
#define MAX_BODY_SIZE 2048

struct HttpParams {
	char path[MAX_HEADER_SIZE];
	char host[MAX_HEADER_SIZE];
	char method[30];
	char protocol[30];
};

struct HttpAnswer {
	int content_lengh;
	char status[4];
	char body[MAX_BODY_SIZE];
	char header[MAX_HEADER_SIZE];
};

struct List {
	char str[MAX_HEADER_SIZE];
	struct List* next;
};

void divide_body(char* response, struct HttpAnswer* answer) {
	char *tmp;
	tmp = strstr(response, "\r\n\r\n");
	if(tmp != NULL){
		strcpy(answer->body, tmp + strlen("\r\n\r\n"));
		strncpy(answer->header, response, (tmp-response));
		response[tmp-response]='\0';
	}
//	printf("head: %s\nbody: %s\n", answer->header, answer->body);
}


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
        //        printf("arg  is: %s\n", ptr);
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
          //      printf("arg  is: %s\n", ptr);
		if(flag == 1)
			remote_answer->content_lengh = atoi(ptr);
                if(strcmp(ptr, "Content-Length:") == 0)
                        flag = 1;
                if(i == 1 && is_first == 1)
                         strcpy(remote_answer->status, ptr);
		i++;
                ptr = strtok(NULL, " ");
        }
}


void parse_request(char* request, struct HttpParams* response, struct HttpAnswer* remote_answer) {
	int i = 0, flag = 0;
	char* ptr;
	char tmp[MAX_HEADER_SIZE];

	struct List* head;
	head = (struct List*)malloc(sizeof(struct List));
	head->next = NULL;

	struct List* cur = head;

	ptr = strtok(request, "\r\n");

//	printf("start parsing\n\n");
	while(ptr != NULL) {
	//	printf("line is: %s\n", ptr);
		struct List* el;
		el = (struct List*)malloc(sizeof(struct List));
		strcpy(el->str, ptr);
		el->next = NULL;
		cur->next = el;
		cur = el;

		ptr = strtok(NULL, "\r\n");
	}

	head = head->next;
	while(head != NULL) {
	//	printf("tmp : %s\n", head->str);
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
			 if(remote_answer != NULL)
                                pars_answer_line(head->str, remote_answer, 0);
		}
		cur = head;
		head = head->next;
		free(cur);
	}

//	printf("end pars\n\n");
	if(response != NULL)
		pars_path(response);
/*
	printf("host - %s\n", response->host);
	printf("path - %s\n", response->path);
	printf("protocol - %s\n", response->protocol);
	printf("method - %s\n", response->method);
*/
}


//pars if we have port
int get_remote_socket(char* host, char* port) {
	int remote_port = 80, sc, res;
	struct sockaddr_in sc_addr;

	if(strlen(port) > 0)
		remote_port = atoi(port);
	struct hostent *hp;
	hp = gethostbyname(host);
	
	if(hp == NULL) {
		printf("wrong url\n");
		return -1;
	}

//	struct in_addr* adr = ((struct in_addr**)(hp -> h_addr_list))[0];
	sc = socket(AF_INET, SOCK_STREAM, 0);
	if(sc < 0) {
		printf("can't create remote socket\n");
		return -1;
	}
	sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = ((struct in_addr*)((hp->h_addr_list)[0]))->s_addr;
	printf("remote_adr is: %s\n", inet_ntoa(sc_addr.sin_addr));

	sc_addr.sin_port = htons(remote_port);
	bzero(&(sc_addr.sin_zero), 8);
        if((res = connect(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr))) < 0) {
		printf("can't connect to the remote socket\n");
		return -1;
	}

	return sc;
}


void form_http_request(struct HttpParams* response, char* header) {
	//char header[MAX_HEADER_SIZE*3] = "";
	/*strcpy(header, arg[0]);
	strcpy(header, " ");
	strcpy(header, path);
	strcpy(header, " ");
	strcpy(header, arg[2]);
	strcpy(header, "\r\nHost: ");
	strcpy(header, host);
	strcpy(header, "\r\nConnection : close\r\n");
	*/
	//sprintf(header, "%s %s %s\r\nHost: %s\r\nConnection: close\r\n", response->method, response->path, response->protocol, response->host);
	sprintf(header, "%s %s %s\r\nHost: %s\r\n\r\n", response->method, response->path, response->protocol, response->host);

//	printf("header: %s\n", header);
}

int main(int argc, char* argv[]) {
	int i, sc, client, remote_host;
	struct sockaddr_in sc_addr;
	char buf[MAX_HEADER_SIZE + MAX_BODY_SIZE];
	int addrlen = sizeof(sc_addr);

//	sc = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	sc = socket(AF_INET, SOCK_STREAM, 0);
	if(sc == -1) {
		printf("can't create socket\n");
		return 1;
	}

	int opt = 1;
	if (setsockopt(sc, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { 
        	perror("setsockopt"); 
        	exit(-1); 
    	} 	


	sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sc_addr.sin_port = htons(5000);
	if(bind(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr)) < 0) {
		printf("can't bind");
		return -1;
	}

	//printf("binded: %d\n", sc);
	if(listen(sc, SOMAXCONN) < 0) {
		printf("errir listen\n");
		return -1;
	}

	//---
	if((client = accept(sc, (struct sockaddr*)&sc_addr, (socklen_t*)&addrlen)) < 0) {
		printf("error accept\n");
		return -1;
	}
	//printf("accepted - %d\n", client);
	//read(sc, buf, 2048);
	int readen = recv(client, buf, 2048, 0);
	if(readen < 0) {
		printf("error reading\n");
		return -1;
	}
	printf("%s\n\n", buf);

	struct HttpParams* param;
	param = (struct HttpParams*)malloc(sizeof(struct HttpParams));

	parse_request(buf, param, NULL);
	remote_host = get_remote_socket(param->host, "");
	if(remote_host < 0) {
		return -1;
	}
	char* send_this;
	send_this = (char*)malloc(sizeof(char)*3*MAX_HEADER_SIZE);
	printf("form hhtp request\n\n");
	form_http_request(param, send_this);

	printf("send this: %s\n", send_this);
	if(write(remote_host, send_this, strlen(send_this)) < 0) {
		printf("can't write to remote host\n");
		return -1;
	}
	
	//if chunked???
	int try_read = 0;
	//copy buf??
	readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
	if(readen < 0) {
		printf("error reading from remote host");
		return -1;
	}
	
	if(write(client, buf, strlen(buf)) < 0) {
                printf("can't write to remote host\n");
                return -1;
        }

	struct HttpAnswer* ans;
	ans = (struct HttpAnswer*)malloc(sizeof(struct HttpAnswer));
	divide_body(buf, ans);


	try_read+=strlen(ans->body);
//	printf("readen bytes - %d and body - %d\n", readen, strlen(ans->body));
	parse_request(ans->header, NULL, ans);
//	printf("content-len: %d answer %s\n", ans->content_lengh, ans->status);

	while(try_read < ans->content_lengh) {
		 readen = read(remote_host, buf, MAX_HEADER_SIZE+MAX_BODY_SIZE);
	        if(readen < 0) {
        	        printf("error reading from remote host-additional read\n");
               		return -1;
        	}
		if(readen == 0)
			break;
		try_read += readen;
		printf("%s", buf);
	}

//	printf("check sum : %d shoul be %d\n", try_read, ans->content_lengh);
	
	free(param);
	free(ans);
	close(sc);
	return 0;
}
