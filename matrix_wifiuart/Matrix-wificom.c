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
#include "pthread.h"
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
int sockfd, clifd, uartfd;
struct sockaddr_in servaddr, cliaddr;



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
	if (comport==1)
	{
		fd = open("/dev/ttySAC0",O_RDWR|O_NOCTTY|O_NDELAY);
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
		fd = open("/dev/ttySAC1",O_RDWR|O_NOCTTY|O_NDELAY);
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
		fd = open("/dev/ttySAC2",O_RDWR|O_NOCTTY|O_NDELAY);
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
		fd = open("/dev/ttySAC3",O_RDWR|O_NOCTTY|O_NDELAY);
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

static void* pthread_UART2WiFi (void* data)
{
	unsigned char buff[128];
	int nread,nsend;

	while (1)
		{
			nread = read(uartfd,buff,256);
			if(nread>0)
			{
				printf("Send Messag %s\n", buff);
				nsend = send(clifd, buff, nread, 0);
				printf(" socket Send number %d \n", nsend);
			}

		}


}

void wifi2uart(int sockfd){
	int n;
	char mesg[MAXLINE];

	for(;;){
		n = recv(clifd, mesg,4096,0);
		if(n>0){
			printf("Receive Messag %s\n", mesg);
			write(uartfd,mesg,n);
		}

        /*
		if((ret = strstr(mesg,"open" )) != NULL){
			 operateLed(true);
			printf("open\n");
		}else if((ret = strstr(mesg,"close" )) != NULL){
			 operateLed(0);
			printf("close\n");
		}*/
		memset(mesg, 0 , sizeof(mesg));

	}
}

int main(void){

	int ret = -1;
	int i = 0;
	pthread_t pt_1 = 0;
	size_t len =  sizeof(struct sockaddr);;
	/*Init Led port*/
	if((ret = exportGPIOPin(led0.pin)) == -1){
		printf("exportGPIO(%d)failed\n", led0.pin);
	}
	if((ret = setGPIODirection(led0.pin, GPIO_OUT)) == -1){
		
		printf("setGPIODirection(%d)failed\n", led0.pin);
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

	if(bind(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) == -1){
		perror("bind error");
		exit(1);
	}
	if(listen(sockfd,10)<0){  
		perror("listen");  
		exit(1);  
	}
	//Initalize UART2
	if((uartfd=open_port(uartfd,4)) < 0)//打开串口 1
	{
		perror("open_port error1\n");
		exit(1);
	}
	if((i=set_opt(uartfd,9600,8,'N',1)) < 0)//设置串口 9600 8 N 1
	{
		perror("set_opt error1\n");
		exit(1);
	}

	clifd = accept(sockfd, (struct sockaddr *)&cliaddr, (int *)&len);
	if(clifd < 0){
		perror("error accept\n");
	}
	//Create Thread to send CMD from uart to wifi
	ret = pthread_create (&pt_1, NULL, pthread_UART2WiFi, NULL);
	if (ret != 0)
	{
	 perror ("pthread_1_create");
	}
	//Receive data from wifi to uart
	wifi2uart(sockfd);

	//wait pthread
	pthread_join (pt_1, NULL);
	close(clifd);
	close(sockfd);
	return 0;
}


