#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>

void parse_request(char* request, char** response) {
	int i = 1;
	char del[]= " ";
	char* ptr = strtok(reuest, del);
	
	while(pthr != NULL){}
}	



int main(int argc, char* argv[]) {
	int i, sc, conn;
	struct sockaddr_in sc_addr;
	char buf[1024];

	sc = socket(AF_INET, SOCK_STREAM, 0);
	if(sc == -1) {
		printf("can't create socket\n");
		return 1;
	}
	sc_addr.sin_family = AF_INET;
        sc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sc_addr.sin_port = htons(5000);
	bind(sc, (struct sockaddr*)&sc_addr, sizeof(sc_addr));

	listen(sc, SOMAXCONN);

	//---
	conn = accept(sc, (struct sockaddr*)NULL, NULL);
	read(sc, buf, sizeof(buf));
	char** response;
	response = (char**)malloc(sizeof()) 
	
	//close(sc);
	return 0;
}
