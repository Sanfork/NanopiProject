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
#include <sys/poll.h>
#include <time.h>
#include <sys/msg.h>
#include <signal.h>
#include "../../lib/common.h"

/********Define************/
#define MAXRECVBUFF 1024
#define SERV_PORT 8888
#define BUFSIZE 8
#define HCSR04DEVICE "/dev/hcsr04"
#define PWMDEVICE "/dev/pwm"
#define MOVEHZ  1000
#define MOVEDUTY 500
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
int sockfd, pwmfd, clifd,forwardPin,backwardPin,leftPin,rightPin,ultrasonicPin;
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
static int setforward(void);
static int stopforward(void);
static int setbackward(void);
static int stopbackward(void);
/*
static void pthread_traction(int parameter){
	pthread_detach(pthread_self());//detach thread
	while(1);
	pthread_exit(NULL);
}*/
static int setforward(){
    int ret; 
    ret = PWMPlay(PWM0, MOVEHZ, MOVEDUTY);
    return ret;
}

static int stopforward(){
    int ret;
    ret = PWMStop(PWM0);
    return ret;
}

static int setbackward(){ 
    int ret;
    ret = PWMPlay(PWM1, MOVEHZ, MOVEDUTY);
    return ret;
}

static int stopbackward(){
    int ret;
    ret = PWMStop(PWM1);
    return ret;
}
 
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

			ret = stopbackward();
                        //setGPIOValue(backwardPin, GPIO_LOW);
			if (ret == -1) {
				printf("stop backward failed\n");
			}
                        usleep(500000);
			ret = setforward();
                         //setGPIOValue(forwardPin, GPIO_HIGH);
			if (ret == -1) {
				printf("set forward failed\n");
			}
			printf("SET UPSTART\n");

		 } else if(msgData.data == danger){
			 printf(" danger\n");
			ret = stopforward();
                        //setGPIOValue(forwardPin, GPIO_LOW);
			if (ret == -1) {
				printf("stop forward failed\n");
			}
                        usleep(500000);
			ret = setbackward();
			//ret = setGPIOValue(backwardPin, GPIO_HIGH);
			if (ret == -1) {
				printf("Set backward failed\n");
			}
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
			                result = setforward();	
                                 //	result = setGPIOValue(forwardPin, GPIO_HIGH);
			        if (result == -1) {
			           // printf("setGPIOValue(%d) failed\n", forwardPin);
                                      printf("setforward fail\n");
			        }
					printf("SET UPSTART\n");
				}else if((ret = strstr(mesg,"UPSTOP" )) != NULL){
				//	result = setGPIOValue(forwardPin, GPIO_LOW);
                                        result = stopforward();
			        if (result == -1) {
			           // printf("setGPIOValue(%d) failed\n", forwardPin);
                                      printf("stopforward fail\n");
			        }
					printf("SET UPSTOP\n");
				}else if((ret = strstr(mesg,"BACKSTART" )) != NULL){
					result = setbackward();
                                        //setGPIOValue(backwardPin, GPIO_HIGH);
			        if (result == -1) {
			            printf("set backward fail\n");
			        }
					printf("SET BACKSTART\n");
				}else if((ret = strstr(mesg,"BACKSTOP" )) != NULL){
					result = stopbackward();
                                         //setGPIOValue(backwardPin, GPIO_LOW);
			        if (result == -1) {
			            printf("stop backward failed\n");
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
	int fd;
	char buff[255];
	int distance, msgid,msgqid,msgrealid;
	struct msg_st msgdata;


	ultrasonicKillFlag = false;
	memset(buff, 0, sizeof(buff));
	printf("pthread_ultralsonic start\n");

	fd = open(HCSR04DEVICE,O_RDWR);
	if(fd == -1){
		printf("open device fail\n");
		goto end1;
	}

	//register kill signal function
	signal(SIGALRM,ultralsonicPthreadKill);
	//Create message queue
	msgid = msgget(key, IPC_EXCL);
	if(msgid < 0){
		msgqid = msgget(key, 0666 | IPC_CREAT );
		if (msgqid < 0){
		printf("fail to creat msg\n");
		goto end2;
		} else {
		msgrealid = msgqid;
		}
	}else {
		msgrealid = msgid;
	}
	msgdata.msg_type = 1;

	while(1){
		read(fd, buff, sizeof(buff));
		distance = atoi(buff);
		distance = (int)(distance / 58);
	        //printf("distance is %d", distance);
		if(distance <100 && distance > 0){
			if(carDistance == safe){
				msgdata.data=danger;
				if(msgsnd(msgrealid, (void*)&msgdata, BUFSIZE, 0) == -1)
				{
					printf( "msgsnd failed\n");
					goto end2;
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
					goto end2;
				}
				printf("distance is %d cm\n",distance);
				carDistance = safe;
			}
		}
		if(ultrasonicKillFlag == true) break;
	}
end2:
	close(fd);
end1:
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

        //initialize PWM   
       // pwmfd = openHW(PWMDEVICE, O_RDONLY);
       // if(pwmfd == -1){
       //     perror("open pwm fail\n"); 
       //     exit(1);

        //} 
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
