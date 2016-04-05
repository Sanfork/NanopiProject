/*
 * matrix-Wificom.c
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "libfahw.h"

typedef enum boolbean {
	false,
	true
}BOOL;

typedef struct {
	int pin;
	BOOL onOff;
}LED_INFO;

#define MAXLINE 5
#define SERV_PORT 8888

LED_INFO led0 = {
	.pin = GPIO_PIN2,
	.onOff = false,
};
int sockfd, clifd;
struct sockaddr_in servaddr, cliaddr;

void operateLed(BOOL operate){
	int ret = -1;
	if (operate){
		ret = setGPIOValue(led0.pin, GPIO_HIGH);
		printf("high\n");
	} else {
		ret = setGPIOValue(led0.pin, GPIO_LOW);
		printf("low\n");
	}
	if(ret == -1) {
			printf("setGPIOValue(%d)failed\n", led0.pin);
	} else led0.onOff = operate; 

}

void do_echo(int sockfd){
	int n;
	struct sockaddr_in cliaddr;
	char *ret;
	size_t len = sizeof(struct sockaddr);
	char mesg[MAXLINE];
	clifd = accept(sockfd, (struct sockaddr *)&cliaddr, (int *)&len);
	if(clifd < 0){
		perror("error accept\n");
	}
	for(;;){
		n = recv(clifd, mesg,4096,0);
		printf("Receive Messag %s\n", mesg);
		

		if((ret = strstr(mesg,"open" )) != NULL){
			 operateLed(true);
			printf("open\n");
		}else if((ret = strstr(mesg,"close" )) != NULL){
			 operateLed(0);
			printf("close\n");
		}
		memset(mesg, 0 , sizeof(mesg));

	}
}

int main(void){

	int ret = -1;
	/*Init Led port*/
	if((ret = exportGPIOPin(led0.pin)) == -1){
		printf("exportGPIO(%d)failed\n", led0.pin);
	}
	if((ret = setGPIODirection(led0.pin, GPIO_OUT)) == -1){
		
		printf("setGPIODirection(%d)failed\n", led0.pin);
	}



	if((sockfd = socket(AF_INET, SOCK_STREAM,0))==-1){
	perror("socket");
	exit(1);	
	};

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	if(bind(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
		perror("bind error");
		exit(1);
	}
	if(listen(sockfd,10)<0){  
		perror("listen");  
		exit(1);  
	}
	do_echo(sockfd);
	return 0;
}


