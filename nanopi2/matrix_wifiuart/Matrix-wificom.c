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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <asm/param.h>
#include <termios.h>
#include <pthread.h>
#include "libfahw.h"
#include <semaphore.h>
#include <assert.h>


#define MAXRECVBUFF 1024
#define SERV_PORT 8888

typedef enum boolbean {
	false,
	true
}BOOL;


typedef struct client_node{
	struct client_node *nextClient;
	struct client_node *preClient;
	int clientId;
}CLIENTSOCKECT_NODE, *pCLIENTSOCKECT_NODE;

int sockfd, clifd, uartfd;
struct sockaddr_in servaddr, cliaddr;
CLIENTSOCKECT_NODE *clientListHead, *clientListTail;
sem_t uart_sem;

/* initialize list */
CLIENTSOCKECT_NODE* initList(void){
	clientListHead = NULL;
	clientListTail = NULL;
	pCLIENTSOCKECT_NODE pHead = (pCLIENTSOCKECT_NODE)malloc(sizeof(CLIENTSOCKECT_NODE));

	pHead->clientId = 0;
	pHead->nextClient = NULL;
	pHead->preClient = NULL;

	return pHead;
}

/*append list*/
void appendList(int data){
	pCLIENTSOCKECT_NODE pNew = NULL;

	if(clientListHead->nextClient == NULL ){/*empty list*/
		clientListHead ->clientId = data;
		clientListHead ->nextClient = clientListHead;
		clientListHead ->preClient = clientListHead;
		clientListTail = clientListHead;
		printf("add first node\n");
	} else {
		pNew = (pCLIENTSOCKECT_NODE)malloc(sizeof(CLIENTSOCKECT_NODE));
		pNew ->preClient = clientListTail;
		pNew ->nextClient = clientListHead;
		pNew ->clientId = data;
		clientListTail ->nextClient = pNew;
		clientListTail = pNew;
		clientListHead->preClient = clientListTail;
		printf("add node\n");
	}
}

/*delete node*/
int deleteNode(int data){
	pCLIENTSOCKECT_NODE i,pointerTmp;
	int ret = -1;

	if(clientListTail == NULL) {
		printf("list is empty\n");
		return -1; /*list is empty*/	
	}


	for(i = clientListHead;;){
		if(i->clientId == data){
			if(i == clientListHead){/*the node is head*/
				if(clientListHead ->nextClient == clientListHead){/*only one node*/
					clientListHead->clientId = 0;
					clientListHead->nextClient = NULL;
					clientListHead->preClient = NULL;
					clientListTail = NULL;
					printf("node is only head\n");
					ret = 0;
					break;
				}else{
					pointerTmp = i->nextClient;
					if(pointerTmp->nextClient == clientListHead){
						/*Two nodes*/
						printf("two nodes\n");
						pointerTmp->nextClient = clientListTail;
					}
					clientListHead = i->nextClient;
					pointerTmp->preClient = clientListTail;

					free(i);
					printf("node is head\n");
					ret = 0;
					break;
				}
			}else if(i == clientListTail){/*The node is tail*/
				pointerTmp = i->nextClient;
				clientListTail = i->preClient;
				clientListTail->nextClient = clientListHead;
				clientListHead->preClient = clientListTail;
				printf("node is tail\n");
				free(i);
				ret = 0;
				break;
			}else{
				pointerTmp = i->preClient;
				pointerTmp ->nextClient = i->nextClient;
				pointerTmp = i->nextClient;
				pointerTmp ->preClient = i ->preClient;
				printf("node is among\n");
				free(i);
				ret = 0;
				break;
			}
		} else {
			if(i->nextClient == clientListHead) break;
			i = i->nextClient;
		}
	}
	return ret;
}

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	if ( tcgetattr( fd,&oldtio) != 0)
	{
		perror("SetupSerial 1");
		return -1;
	}
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;
	switch( nBits )
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
		break;
	}
	switch( nEvent )
	{
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E':
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;
		case 'N':
			newtio.c_cflag &= ~PARENB;
			break;
	}
	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
		break;
	}
	if( nStop == 1 )
		newtio.c_cflag &= ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |= CSTOPB;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	printf("set done!\n");
	return 0;
}

int open_port(int fd,int comport)
{
	//char *dev[]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2"};
	//long vdisable;//没用
	if (comport==1)
	{
		fd = open("/dev/ttyAMA0",O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
			perror("Can't Open UART0");
			return(-1);
		}
		else
			printf("open UART0 .....\n");
		}
	else if(comport==2)
	{
		fd = open("/dev/ttyAMA1",O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
			perror("Can't Open UART1");
			return(-1);
		}
		else
			printf("open UART1 .....\n");
		}
	else if (comport==3)
	{
		fd = open("/dev/ttyAMA2",O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
		perror("Can't Open UART2");
		return(-1);
		}
		else
		printf("open UART2 .....\n");
	}
	else if (comport==4)
	{
		fd = open("/dev/ttyAMA3",O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd)
		{
		perror("Can't Open ttySAC3");
		return(-1);
		}
		else
		printf("open UART3 .....\n");
	}
	if(fcntl(fd, F_SETFL, 0) < 0)
		printf("fcntl failed!\n");
	else
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));

	if(isatty(STDIN_FILENO)==0)
		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");

		printf("fd-open=%d\n",fd);
		return fd;
}



void pthread_UART2WiFi (void* data)
{
	unsigned char buff[256];
	pCLIENTSOCKECT_NODE i;
	int sockt,nread;

	while (1)
		{
			nread = read(uartfd,buff,256);
			if(nread>0)
			{
				i = clientListHead;
				while(1){
					sockt = i->clientId;
					send(sockt, buff, nread, 0);
					if(i->nextClient == clientListHead) break;
					i = i->nextClient;
				}
			}
		}
}

void wifi2uart(int clien_fd){
	int n;
	char ret;

	char mesg[MAXRECVBUFF];

	for(;;){
		n = recv(clien_fd, mesg,4096,0);
		if(n <= 0){
			if(n == EINTR){
				continue; /*Connectiong is normal, communication has abnormal*/
			} else {
				printf("client disconnect\n");
				ret = deleteNode(clien_fd);
				if(ret !=0) {
					perror("Delete node failure");
					break;
				}
				ret = close(clien_fd);
				if(ret !=0) {
					perror("Close client failure");
					break;
				}
				break;
			}
		}else{
			sem_wait(&uart_sem);
			printf("Receive Messag %s\n", mesg);


			write(uartfd,mesg,strlen(mesg));
			sem_post(&uart_sem);
			memset(mesg, 0 , sizeof(mesg));
		}


	}
}

void thread_wifiUart(int clientFd){
	int fd = *((int *) clientFd);

	//Receive data from wifi to uart
	wifi2uart(fd);

	pthread_exit(NULL);

}

int main(void){

	int ret = -1;
	pthread_t thread_uartRec;
	int on = 1;



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
	//Initialize UART2
	if((uartfd=open_port(uartfd,3)) < 0)//打开串口 1
	{
		perror("open_port error1\n");
		exit(1);
	}
	if((ret=set_opt(uartfd,9600,8,'N',1)) < 0)//设置串口 9600 8 N 1
	{
		perror("set_opt error1\n");
		exit(1);
	}
	/*Initialize list*/
	clientListHead = initList();

	/*Intialize semaphore*/
	ret = sem_init(&uart_sem, 0, 1);
	if(ret == -1){
		perror("semaphore create fail!");
		exit(EXIT_FAILURE);
	}
	ret = pthread_create (&thread_uartRec, NULL, (void*)(&pthread_UART2WiFi), NULL);
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

		appendList(clifd);
		/*Create Thread to send CMD from uart to wifi*/
		ret = pthread_create (&thread_id, NULL, (void*)(&thread_wifiUart), (void*)(&clifd));
		if (ret != 0)
		{
		 perror ("pthread_create error!");
		}
	}



	ret = shutdown(sockfd,SHUT_WR);
	assert(ret != -1);
	return 0;
}


