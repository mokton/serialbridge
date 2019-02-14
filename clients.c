#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define     MAX_BUFF_SIZE   1024

void *cli1_run(void *cfd);
void *cli1_thread(void *cfd);
void *cli1_thread_send(void *cfd);
void *cli1_thread_recv(void *cfd);

void *cli2_run(void *cfd);
void *cli2_thread(void *cfd);
void *cli2_thread_send(void *cfd);
void *cli2_thread_recv(void *cfd);

char cli1_ip[15];
int  cli1_port = 0;
int  cli1_cancel = 0;
char cli1_rev[MAX_BUFF_SIZE];
int  cli1_length = 0;

char cli2_ip[15];
int  cli2_port = 0;
int  cli2_cancel = 0;
char cli2_rev[MAX_BUFF_SIZE];
int  cli2_length = 0;

int main(int argc, char **argv){
	
    if(argc<3){
        printf("Usage: %s IP1:PORT1 IP2:PORT2\n", argv[0]);
        return 0;
    }

    sscanf(argv[1], "%[^:]:%d", cli1_ip, &cli1_port);
    sscanf(argv[2], "%[^:]:%d", cli2_ip, &cli2_port);
    printf("Link %s:%d to %s:%d\n", cli1_ip, cli1_port, cli2_ip, cli2_port);
    //return 0;

	while(1){
		//消息处理
		pthread_t tid1, tid2;
		void* ret1;
		void* ret2;

        pthread_create(&tid1, NULL, cli1_run, (void *)0);	
        pthread_create(&tid2, NULL, cli2_run, (void *)0);
        pthread_join(tid1, &ret1);
        pthread_join(tid2, &ret2);
        
        usleep(100); // 100 microsecond
	}
	return 0;
}


//=====================================================
void *cli1_run(void *arg){
	struct sockaddr_in serv;
	int sfd, ct;
    sfd=socket(AF_INET,SOCK_STREAM,0);//创建一个通讯端点
		if(sfd==-1){
			perror("Client1 socket error");
			pthread_exit((void *) -1);
		}
	//初始化结构体serv
    serv.sin_family=AF_INET;
    serv.sin_port=htons(cli1_port);
    inet_pton(AF_INET,cli1_ip,&serv.sin_addr);

	while(1){
		//消息处理
		pthread_t tid;
		void* ret;
        sfd=socket(AF_INET,SOCK_STREAM,0);//创建一个通讯端点
		if(sfd>=0){
			ct=connect(sfd,(struct sockaddr *)&serv,sizeof(serv));
            if(ct==-1){
                perror("Client1 connect error");
            }else{
                cli1_cancel = 0;
                printf("Client1 socket connected\n");
                pthread_create(&tid,NULL,cli1_thread,&sfd);	
                pthread_join(tid,&ret);
            }
		}

        usleep(100); // 100 microsecond
	}
}

void *cli1_thread(void *cfd){
	void* ret1;
	void* ret2;
	//创建两个线程用于接收和发送消息
	pthread_t tid1,tid2;
	pthread_create(&tid1,NULL,cli1_thread_send,cfd);
	pthread_create(&tid2,NULL,cli1_thread_recv,cfd);	
	pthread_join(tid1,&ret1);
	pthread_join(tid2,&ret2);
}
void *cli1_thread_send(void* cfd){
	char tmp[516];
	int c=*((int *)cfd);
	while(1){
		//fgets(tmp,516,stdin);
        if(cli2_length>0)
        {
		    send(c,cli2_rev,cli2_length,0);
            memset(cli2_rev, 0, MAX_BUFF_SIZE);
            cli2_length=0;
		    printf("Client1 send ok\n");
        }
        if(cli1_cancel)
            pthread_exit((void *) 1);

        usleep(100); // 100 microsecond
	}
}
void *cli1_thread_recv(void* cfd){
	char tmp[516];
	int d=*((int *)cfd);
    struct tcp_info info;
    int infoLen=sizeof(info);
    int errCount=0;
    int i=0;
    //getsockopt(d, SOL_TCP, TCP_INFO, &info, (socklen_t *)&infoLen); //IPPROTO_TCP
	while(1){
		//int idata=0;
        getsockopt(d, SOL_TCP, TCP_INFO, &info, (socklen_t *)&infoLen);
        if((info.tcpi_state!=TCP_ESTABLISHED))
        {
            errCount++;
            if(errCount>10)
            {
                printf("Client1 link break\n");
                shutdown (d, 2);
                cli1_cancel = 1;
                pthread_exit((void *) 1);
            }
        }
		//idata=recv(d,tmp,516,0);
		cli1_length=recv(d,cli1_rev,MAX_BUFF_SIZE,0);
		if(cli1_length>0){
			printf("Client1 recv (%d): ", cli1_length);
			//printf("%s \n",cli1_rev);
            for(i=0; i<cli1_length; i++)
                printf("%02x ", cli1_rev[i]);
                printf("\n");
		}//else
         //   printf("socket disconnected\n");
        
        usleep(100); // 100 microsecond
	}
}

//===================================================

void *cli2_run(void *arg){
	struct sockaddr_in serv;
	int sfd, ct;
    sfd=socket(AF_INET,SOCK_STREAM,0);//创建一个通讯端点
		if(sfd==-1){
			perror("Client2 socket error");
			pthread_exit((void *) -1);
		}
	//初始化结构体serv
    serv.sin_family=AF_INET;
    serv.sin_port=htons(cli2_port);
    inet_pton(AF_INET,cli2_ip,&serv.sin_addr);

	while(1){
		//消息处理
		pthread_t tid;
		void* ret;
        sfd=socket(AF_INET,SOCK_STREAM,0);//创建一个通讯端点
		if(sfd>=0){
			ct=connect(sfd,(struct sockaddr *)&serv,sizeof(serv));
            if(ct==-1){
                perror("Client2 connect error");
            }else{
                cli1_cancel = 0;
                printf("client2 socket connected\n");
                pthread_create(&tid,NULL,cli2_thread,&sfd);	
                pthread_join(tid,&ret);
            }
		}
        
        usleep(100); // 100 microsecond
	}
}

void *cli2_thread(void *cfd){
	void* ret1;
	void* ret2;
	//创建两个线程用于接收和发送消息
	pthread_t tid1,tid2;
	pthread_create(&tid1,NULL,cli2_thread_send,cfd);
	pthread_create(&tid2,NULL,cli2_thread_recv,cfd);	
	pthread_join(tid1,&ret1);
	pthread_join(tid2,&ret2);
}
void *cli2_thread_send(void* cfd){
	char tmp[516];
	int c=*((int *)cfd);
	while(1){
		//fgets(tmp,516,stdin);
        if(cli1_length>0)
        {
		    send(c,cli1_rev,cli1_length,0);
            memset(cli1_rev, 0, MAX_BUFF_SIZE);
            cli1_length=0;
		    printf("Client2 send ok\n");
        }
        if(cli2_cancel)
            pthread_exit((void *) 1);
        
        usleep(100); // 100 microsecond
	}
}
void *cli2_thread_recv(void* cfd){
	char tmp[516];
	int d=*((int *)cfd);
    struct tcp_info info;
    int infoLen=sizeof(info);
    int errCount=0;
    int i=0;
    //getsockopt(d, SOL_TCP, TCP_INFO, &info, (socklen_t *)&infoLen); //IPPROTO_TCP
	while(1){
		//int idata=0;
        getsockopt(d, SOL_TCP, TCP_INFO, &info, (socklen_t *)&infoLen);
        if((info.tcpi_state!=TCP_ESTABLISHED))
        {
            errCount++;
            if(errCount>10)
            {
                printf("Client2 link break\n");
                shutdown (d, 2);
                cli2_cancel = 1;
                pthread_exit((void *) 1);
            }
        }
		
		cli2_length=recv(d,cli2_rev,MAX_BUFF_SIZE,0);
		if(cli2_length>0){
			printf("Client2 recv (%d): ", cli2_length);
			//printf("%s \n",cli2_rev);
            for(i=0; i<cli2_length; i++)
                printf("%02x ", cli2_rev[i]);
                printf("\n");
		}//else
         //   printf("socket disconnected\n");
        
        usleep(100); // 100 microsecond
	}
}
