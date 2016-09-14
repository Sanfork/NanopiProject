/*
 * matrix-remotecar.c
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
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
#include <time.h>
#include <sys/msg.h>
#include <signal.h>
#include "../../lib/common.h"

/********Define************/
#define MAXRECVBUFF 1024
#define SERV_PORT 8888
#define BUFSIZE 8

typedef enum boolbean {
	false,
	true
}BOOL;

typedef enum direction{
	forward,
	backward,
	forwardleft,
	forwardright,
	backwardleft,
	backwatdright,
	stop
}DIRECTION;

typedef enum distans{
	safe,
	danger,
}DISTANCE;

typedef enum operationMode{
	stopmode,
	autoMode,
	menuMode,
	tractionMode,
}OPERATIONMODE;

struct msg_st{
    long int msg_type;
    int data;
};

/********Variable*************/
int sockfd, clifd,forwardPin,backwardPin,leftPin,rightPin,ultrasonicPin;
DIRECTION carDirection;
DISTANCE  carDistance;
key_t key;
pthread_t autoModeThread,wifiThread,ultrasonicThread,tractionThread;
OPERATIONMODE mode;
BOOL autoModeKillFlag,ultrasonicKillFlag;
struct sockaddr_in servaddr, cliaddr;

/**********FUNCTION********/
static void receiveCommand(int clien_fd);
static void pthread_autoMode(int parameter);
static void pthread_wifiCon(int clientFd);
static void pthread_ultralsonic(int parameter);
static void autoModePthreadKill(int sig);
static void ultralsonicPthreadKill(int sig);
/*
static void pthread_traction(int parameter){
	pthread_detach(pthread_self());//detach thread
	while(1);
	pthread_exit(NULL);
}*/

static void ultralsonicPthreadKill(int sig){
	ultrasonicKillFlag = true;
}

static void autoModePthreadKill(int sig){
	pthread_kill(ultrasonicThread,SIGALRM);
	autoModeKillFlag = true;
}

static void pthread_autoMode(int parameter){
	int ret;
	int msgid = -1;
	int msgqid, msgrealid;
	struct msg_st msgData;
	long int msgtype = 0;

	mode = autoMode;
	autoModeKillFlag = false;

	pthread_detach(pthread_self());//detach thread

	//register kill signal function
	signal(SIGALRM, autoModePthreadKill);

	/*Create Thread to create ultrasonic*/
	if(ultrasonicThread == 0){
		ret = pthread_create (&ultrasonicThread, NULL, (void*)(&pthread_ultralsonic), NULL);
		if (ret != 0)
		{
		 perror ("pthread_create error!");
		}
	}else{
		//TODO: wakeup ultrasonic thread
	}

	/* Car start to move forward*/
	ret = setGPIOValue(forwardPin, GPIO_HIGH);
	if (ret == -1) {
		printf("setGPIOValue(%d) failed\n", forwardPin);
	}
	printf("SET UPSTART\n");
	 msgid = msgget(key, IPC_EXCL);
	 if(msgid < 0){
		 msgqid = msgget(key, 0666 | IPC_CREAT );
		 if (msgqid < 0){
			 printf("fail to creat msg\n");
			 pthread_exit(NULL);
		 } else {
			 msgrealid = msgqid;
		 }
	 }else {
		 msgrealid = msgid;
	 }

	 while(1){
		 if(msgrcv(msgrealid,(void*)&msgData,BUFSIZE,msgtype,0) == -1 ){
			 printf("message receive fail \n");
			 break;
		 }
		 printf("Receive: %d\n", msgData.data);

		 if(msgData.data == safe){
			printf(" safe \n");
			ret = setGPIOValue(forwardPin, GPIO_HIGH);
			if (ret == -1) {
				printf("setGPIOValue(%d) failed\n", forwardPin);
			}
			printf("SET UPSTART\n");

		 } else if(msgData.data == danger){
			 printf(" danger\n");
		 }

		 if(autoModeKillFlag == true) break;
	 }

	pthread_exit(NULL);

}


static void receiveCommand(int clien_fd){
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
			if((ret = strstr(mesg,"AUTO" )) != NULL){
				/*Create Thread to create automode pthread*/
				if(autoModeThread == 0) {
					result = pthread_create (&autoModeThread, NULL, (void*)(&pthread_autoMode), NULL);
					if (ret != 0)
					{
					 perror ("pthread_create error!");
					}

				} else {
					//TODO: wakeup automode thread
				}


			}else {
				if((ret = strstr(mesg,"MENU" )) != NULL){
					mode = menuMode;
					//stop automode thread
					if(autoModeThread != 0){
						result = pthread_kill(autoModeThread, SIGALRM);
						if(ret != 0) {
							printf("pthread automode cancel fail!\n");
						} else {
							autoModeThread = 0;
						}
					}


				}else if((ret = strstr(mesg,"UPSTART" )) != NULL){
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
				}else if((ret = strstr(mesg,"LEFTUPSTART" )) != NULL){
					result = setGPIOValue(leftPin, GPIO_HIGH);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", leftPin);
			        }
					result = setGPIOValue(forwardPin, GPIO_HIGH);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", forwardPin);
			        }

					printf("SET LEFTUPSTART\n");
				}else if((ret = strstr(mesg,"LEFTUPSTOP" )) != NULL){
					result = setGPIOValue(leftPin, GPIO_LOW);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", leftPin);
			        }
					result = setGPIOValue(forwardPin, GPIO_LOW);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", forwardPin);
			        }
			        printf("SET LEFTUPSTOP\n");
				}else if((ret = strstr(mesg,"LEFTBACKSTART" )) != NULL){
					result = setGPIOValue(leftPin, GPIO_HIGH);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", leftPin);
			        }
					result = setGPIOValue(backwardPin, GPIO_HIGH);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", backwardPin);
			        }

					printf("SET LEFTBACKSTART\n");
				}else if((ret = strstr(mesg,"LEFTBACKSTOP" )) != NULL){
					result = setGPIOValue(leftPin, GPIO_LOW);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", leftPin);
			        }
					result = setGPIOValue(backwardPin, GPIO_LOW);
			        if (result == -1) {
			            printf("setGPIOValue(%d) failed\n", backwardPin);
			        }
			        printf("SET LEFTBACKSTOP\n");

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
				}else if((ret = strstr(mesg,"RIGHTUPSTART" )) != NULL){
					result = setGPIOValue(rightPin, GPIO_HIGH);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", rightPin);
					}
					result = setGPIOValue(forwardPin, GPIO_HIGH);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", forwardPin);
					}
					printf("SET RIGHTUPSTART\n");
				}else if((ret = strstr(mesg,"RIGHTUPSTOP" )) != NULL){
					result = setGPIOValue(rightPin, GPIO_LOW);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", rightPin);
					}
					result = setGPIOValue(forwardPin, GPIO_LOW);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", forwardPin);
					}
					printf("SET RIGHTUPSTOP\n");
				}else if((ret = strstr(mesg,"RIGHTBACKSTART" )) != NULL){
					result = setGPIOValue(rightPin, GPIO_HIGH);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", rightPin);
					}
					result = setGPIOValue(backwardPin, GPIO_HIGH);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", backwardPin);
					}
					printf("SET RIGHTBACKSTART\n");
				}else if((ret = strstr(mesg,"RIGHTBACKSTOP" )) != NULL){
					result = setGPIOValue(rightPin, GPIO_LOW);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", rightPin);
					}
					result = setGPIOValue(backwardPin, GPIO_LOW);
					if (result == -1) {
						printf("setGPIOValue(%d) failed\n", backwardPin);
					}
					printf("SET RIGHTBACKSTOP\n");
				}
			}


			memset(mesg, 0 , sizeof(mesg));
		}


	}
}

static void pthread_wifiCon(int clientFd){
	int fd = *((int *) clientFd);

	pthread_detach(pthread_self());//detach thread

	//Receive data from wifi to uart
	receiveCommand(fd);

	pthread_exit(NULL);

}

static void pthread_ultralsonic(int parameter){

	char ultrasonicEchoPath[60] = "/sys/class/gpio/gpio";
	char ultrasonicEchoPathEdge[60];
	char ultrasonicEchoPathValue[60];
	char *ultrasonicEchoStr;
	char buff[10];
	int ret, ultrasonicEcho ,ultrasonicEchoPinNum,ultrasonicEcho_fd;
    struct pollfd fds[1] ;
	struct timespec time_start={0,0},time_end={0,0};
	long fCostTime;
	int distance, msgid,msgqid,msgrealid;
	struct msg_st msgdata;


	ultrasonicKillFlag = false;
	ultrasonicPin = GPIO_PIN(12);
	ultrasonicEcho = GPIO_PIN(18);
	printf("pthread_ultralsonic start\n");

	//register kill signal function
	signal(SIGALRM,ultralsonicPthreadKill);

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
	ultrasonicEchoStr = (char *)malloc(sizeof(int) + 1);  //锟斤拷锟戒动态锟节达拷
	memset(ultrasonicEchoStr, 0, sizeof(int) + 1);              //锟节达拷锟斤拷始锟斤拷
	sprintf(ultrasonicEchoStr, "%d", ultrasonicEchoPinNum);      //锟斤拷锟斤拷转锟斤拷为锟街凤拷锟斤拷

	//strcpy(ultrasonicEchoStr,"gpio"+ultrasonicEchoStr);
	strcat(ultrasonicEchoPath,ultrasonicEchoStr);//  /sys/class/gpio/gpio18
	printf("ultralsonic PinNumber is %s\n",ultrasonicEchoPath);
	strcpy(ultrasonicEchoPathEdge,ultrasonicEchoPath);
	strcat(ultrasonicEchoPathEdge,"/edge"); //  /sys/class/gpio/gpio18/edge
	printf("ultralsonic path edge is %s\n",ultrasonicEchoPathEdge);
	writeValueToFile(ultrasonicEchoPathEdge, "both");
	strcpy(ultrasonicEchoPathValue,ultrasonicEchoPath);
	strcat(ultrasonicEchoPathValue,"/value");//  /sys/class/gpio/gpio18/value
	printf("ultralsonic path value is %s\n",ultrasonicEchoPathValue);
	ultrasonicEcho_fd = open(ultrasonicEchoPathValue, O_RDONLY);
	if(ultrasonicEcho_fd == -1) printf("Open gpio fail\n");
	fds[0].fd = ultrasonicEcho_fd;
	fds[0].events = POLLPRI;
	ret = read(ultrasonicEcho_fd,buff,10);
	if(ret == -1 ) printf("Read ultrasonicEcho_fd fail\n");

	msgid = msgget(key, IPC_EXCL);
	if(msgid < 0){
		msgqid = msgget(key, 0666 | IPC_CREAT );
		if (msgqid < 0){
		printf("fail to creat msg\n");
		pthread_exit(NULL);
		} else {
		msgrealid = msgqid;
		}
	}else {
		msgrealid = msgid;
	}
	msgdata.msg_type = 1;
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
			//printf("Read ultrasonicEcho_fd %s\n",buff);
			clock_gettime(CLOCK_REALTIME,&time_start);
			//printf("Start time %lu s, %lu ns", time_start.tv_sec, time_start.tv_nsec);

		}

		ret = poll(fds, 1 , -1);
		if(ret == -1) printf("poll fail\n");

		if(fds[0].revents & POLLPRI){
			ret = lseek(ultrasonicEcho_fd,0,SEEK_SET);
			if(ret == -1) printf("Lseek fail\n");
			ret = read(ultrasonicEcho_fd,buff,10);
			if(ret == -1) printf("Read ultrasonicEcho_fd fail\n");
			//printf("Read ultrasonicEcho_fd %s\n",buff);
			clock_gettime(CLOCK_REALTIME,&time_end);
			//printf("Start time %lu s, %lu ns\n", time_end.tv_sec, time_end.tv_nsec);
		}
		fCostTime = (long)(time_end.tv_nsec - time_start.tv_nsec);
		//printf("cost time %lu s\n",fCostTime);
		distance = (fCostTime * 340)/20000000;

		if(distance <100 && distance > 0){
			if(carDistance == safe){
				msgdata.data=danger;
				if(msgsnd(msgrealid, (void*)&msgdata, BUFSIZE, 0) == -1)
				{
					printf( "msgsnd failed\n");
					pthread_exit(NULL);
				}
				printf("distance is %d cm\n",distance);
				carDistance = danger;
			}
		}else{
			if(carDistance == danger){
				msgdata.data=safe;
				if(msgsnd(msgrealid, (void*)&msgdata, BUFSIZE, 0) == -1)
				{
					printf("msgsnd failed\n");
					pthread_exit(NULL);
				}
				printf("distance is %d cm\n",distance);
				carDistance = safe;
			}
		}
		if(ultrasonicKillFlag == true) break;
	}
	pthread_exit(NULL);
}

int main(void){

	int ret = -1;
	int on = 1;
	char* msgpath="/etc/profile";

	//initialize mode
	mode = stop;

	//initialize threadid
	autoModeThread = 0;
	wifiThread = 0;
	ultrasonicThread = 0 ;
	tractionThread = 0;
	//Initalize car status
	carDirection = stop;
	carDistance = safe;

	//Initialize Kill flag
	autoModeKillFlag = false;
	ultrasonicKillFlag =false;

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

	//Initialize message queue
	key = ftok(msgpath,'a');

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
		size_t len = sizeof(struct sockaddr);

		clifd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
		if(clifd < 0){
			perror("error accept\n");
		}

		/*Create Thread to send CMD from uart to wifi*/
		ret = pthread_create (&wifiThread, NULL, (void*)(&pthread_wifiCon), (void*)(&clifd));
		if (ret != 0)
		{
		 perror ("pthread_create error!");
		}
	}



	ret = shutdown(sockfd,SHUT_WR);
	assert(ret != -1);
	return 0;
}
