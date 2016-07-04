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
#include <unistd.h>
#include <sys/poll.h>
#include "../../lib/common.h"

#define MAXRECVBUFF 1024
#define SERV_PORT 8888

typedef enum boolbean {
	false,
	true
}BOOL;


#define SERV_PORT 8888


int sockfd, clifd,forwardPin,backwardPin,leftPin,rightPin,ultrasonicPin;
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

void pthread_ultralsonic(int parameter){

	char ultrasonicEchoPath[60] = "/sys/class/gpio/gpio";
	char ultrasonicEchoPathEdge[60];
	char ultrasonicEchoPathValue[60];
	char *ultrasonicEchoStr;
	char buff[10];
	int ret, ultrasonicEcho ,ultrasonicEchoPinNum,ultrasonicEcho_fd;
        struct pollfd fds[1] ;

	ultrasonicPin = GPIO_PIN(12);
	ultrasonicEcho = GPIO_PIN(18);
	printf("pthread_ultralsonic start\n");
	

	//initialize triger pin	
	if ((ret = exportGPIOPin(ultrasonicPin)) == -1) {
	printf("exportGPIOPin(%d) failed\n", ultrasonicPin);
	}
	if ((ret = setGPIODirection(ultrasonicPin, GPIO_OUT)) == -1) {
	printf("setGPIODirection(%d) failed\n", ultrasonicPin);
	}
	//initialize ultrasonic to sensor
	if ((ret = exportGPIOPin(ultrasonicEcho)) == -1) {
	printf("exportGPIOPin(%d) failed\n", ultrasonicEcho);
	}
	//initialize ultrasonic to sensor
	if ((ret = setGPIODirection(ultrasonicEcho,GPIO_IN)) == -1) {
	printf("exportGPIOPin(%d) failed\n", ultrasonicEcho);
	}

	ultrasonicEchoPinNum = pintoGPIO(GPIO_PIN(18));
	printf("ultralsonic PinNumber is %d\n",ultrasonicEchoPinNum);	
	//itoa(ultrasonicEchoPinNum,ultrasonicEchoStr,10);
	ultrasonicEchoStr = (char *)malloc(sizeof(int) + 1);  //分配动态内存
	memset(ultrasonicEchoStr, 0, sizeof(int) + 1);              //内存块初始化
	sprintf(ultrasonicEchoStr, "%d", ultrasonicEchoPinNum);      //整数转化为字符串

	//strcpy(ultrasonicEchoStr,"gpio"+ultrasonicEchoStr);
	strcat(ultrasonicEchoPath,ultrasonicEchoStr);//  /sys/class/gpio/gpio18
	printf("ultralsonic PinNumber is %s\n",ultrasonicEchoPath);
	strcpy(ultrasonicEchoPathEdge,ultrasonicEchoPath);
	strcat(ultrasonicEchoPathEdge,"/edge"); //  /sys/class/gpio/gpio18/edge

	writeValueToFile(ultrasonicEchoPath, "both");
	strcpy(ultrasonicEchoPathValue,ultrasonicEchoPath);
	strcat(ultrasonicEchoPathValue,"/value");//  /sys/class/gpio/gpio18/value	
	ultrasonicEcho_fd = open(ultrasonicEchoPathValue, O_RDONLY);
	if(ultrasonicEcho_fd == -1) printf("Open gpio fail\n");
	fds[0].fd = ultrasonicEcho_fd;
	fds[0].events = POLLPRI;
	ret = read(ultrasonicEcho_fd,buff,10);
	if(ret == -1 ) printf("Read ultrasonicEcho_fd fail\n");
	
        

	while(1){
		//triger send 60us high 
		if ((ret = setGPIOValue(ultrasonicPin, GPIO_HIGH)) == -1) {
			printf("setGPIODirection(%d)HIGH failed\n", ultrasonicPin);
		}
		usleep(60);
		if ((ret = setGPIOValue(ultrasonicPin, GPIO_LOW)) == -1) {
			printf("setGPIODirection(%d)HIGH failed\n", ultrasonicPin);
		}

		ret = poll(fds, 1 , -1);
		if(ret == -1) printf("poll fail\n");

		if(fds[0].revents & POLLPRI){
		ret = lseek(ultrasonicEcho_fd,0,SEEK_SET);
		if(ret == -1) printf("Lseek fail\n");
		ret = read(ultrasonicEcho_fd,buff,10);
		if(ret == -1) printf("Read ultrasonicEcho_fd fail\n");
		printf("Read ultrasonicEcho_fd %s\n",buff);
		sleep(2);
		}		
	}	
}

int main(void){

	int ret = -1;
	pthread_t thread_id;
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

	thread_id = 0;
	/*Create Thread to create ultrasonic*/
	ret = pthread_create (&thread_id, NULL, (void*)(&pthread_ultralsonic), NULL);
	if (ret != 0)
	{
	 perror ("pthread_create error!");
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
