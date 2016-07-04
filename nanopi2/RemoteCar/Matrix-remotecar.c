/*
 * matrix-remotecar.c
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <asm/param.h>
#include <pthread.h>
#include "libfahw.h"
#include <assert.h>


#define MAXRECVBUFF 1024
#define SERV_PORT 8888

typedef enum boolbean {
	false,
	true
}BOOL;


#define SERV_PORT 8888


int sockfd, clifd,forwardPin,backwardPin,leftPin,rightPin;
struct sockaddr_in servaddr, cliaddr;

void directControl(int clien_fd){
	int n;
	int result = -1;

	char mesg[MAXRECVBUFF];

	for(;;){
		n = recv(clien_fd, mesg,4096,0);
		if(n <= 0){
			if(n == EINTR){
				continue; /*Connectiong is normal, communication has abnormal*/
			} else {
				printf("client disconnect\n");
				result = close(clien_fd);
				if(result !=0) {
					perror("Close client failure");
					break;
				}
				break;
			}
		}else{
			char *ret;
			printf("Receive Messag %s\n", mesg);
			if((ret = strstr(mesg,"UPSTART" )) != NULL){
				result = setGPIOValue(forwardPin, GPIO_HIGH);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", forwardPin);
		        }
				printf("SET UPSTART\n");
			}else if((ret = strstr(mesg,"UPSTOP" )) != NULL){
				result = setGPIOValue(forwardPin, GPIO_LOW);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", forwardPin);
		        }
				printf("SET UPSTOP\n");
			}else if((ret = strstr(mesg,"BACKSTART" )) != NULL){
				result = setGPIOValue(backwardPin, GPIO_HIGH);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", backwardPin);
		        }
				printf("SET BACKSTART\n");
			}else if((ret = strstr(mesg,"BACKSTOP" )) != NULL){
				result = setGPIOValue(backwardPin, GPIO_LOW);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", backwardPin);
		        }
				printf("SET BACKSTOP\n");
			}else if((ret = strstr(mesg,"LEFTSTART" )) != NULL){
				result = setGPIOValue(leftPin, GPIO_HIGH);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", leftPin);
		        }
				printf("SET LEFTSTART\n");
			}else if((ret = strstr(mesg,"LEFTSTOP" )) != NULL){
				result = setGPIOValue(leftPin, GPIO_LOW);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", leftPin);
		        }
				printf("SET LEFTSTOP\n");
			}else if((ret = strstr(mesg,"RIGHTSTART" )) != NULL){
				result = setGPIOValue(rightPin, GPIO_HIGH);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", rightPin);
		        }
				printf("SET RIGHTSTART\n");
			}else if((ret = strstr(mesg,"RIGHTSTOP" )) != NULL){
				result = setGPIOValue(rightPin, GPIO_LOW);
		        if (result == -1) {
		            printf("setGPIOValue(%d) failed\n", rightPin);
		        }
				printf("SET RIGHTSTOP\n");
			}


			memset(mesg, 0 , sizeof(mesg));
		}


	}
}

void pthread_direcCon(int clientFd){
	int fd = *((int *) clientFd);

	pthread_detach(pthread_self());//detach thread

	//Receive data from wifi to uart
	directControl(fd);

	pthread_exit(NULL);

}

int main(void){

	int ret = -1;
	int on = 1;

	//Initializing GPIO
	forwardPin = GPIO_PIN(7);
	backwardPin = GPIO_PIN(11);
	leftPin = GPIO_PIN(13);
	rightPin = GPIO_PIN(15);

	if ((ret = exportGPIOPin(forwardPin)) == -1) {
	printf("exportGPIOPin(%d) failed\n", forwardPin);
	}
	if ((ret = setGPIODirection(forwardPin, GPIO_OUT)) == -1) {
	printf("setGPIODirection(%d) failed\n", forwardPin);
	}

	if ((ret = exportGPIOPin(backwardPin)) == -1) {
	printf("exportGPIOPin(%d) failed\n", backwardPin);
	}
	if ((ret = setGPIODirection(backwardPin, GPIO_OUT)) == -1) {
	printf("setGPIODirection(%d) failed\n", backwardPin);
	}

	if ((ret = exportGPIOPin(leftPin)) == -1) {
	printf("exportGPIOPin(%d) failed\n", leftPin);
	}
	if ((ret = setGPIODirection(leftPin, GPIO_OUT)) == -1) {
	printf("setGPIODirection(%d) failed\n", leftPin);
	}

	if ((ret = exportGPIOPin(rightPin)) == -1) {
	printf("exportGPIOPin(%d) failed\n", rightPin);
	}
	if ((ret = setGPIODirection(rightPin, GPIO_OUT)) == -1) {
	printf("setGPIODirection(%d) failed\n", rightPin);
	}



	//Initialize socket
	if((sockfd = socket(AF_INET, SOCK_STREAM,0))==-1){
		perror("socket");
		exit(1);
	};

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0){
		perror("sockopt error");
		exit(1);
	}

	if(bind(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
		perror("bind error");
		exit(1);
	}
	if(listen(sockfd,10)<0){  
		perror("listen");  
		exit(1);  
	}

	while(1){
		pthread_t thread_id = 0;
		size_t len = sizeof(struct sockaddr);

		clifd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
		if(clifd < 0){
			perror("error accept\n");
		}

		/*Create Thread to send CMD from uart to wifi*/
		ret = pthread_create (&thread_id, NULL, (void*)(&pthread_direcCon), (void*)(&clifd));
		if (ret != 0)
		{
		 perror ("pthread_create error!");
		}
	}



	ret = shutdown(sockfd,SHUT_WR);
	assert(ret != -1);
	return 0;
}
