#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "sockserver.h"
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib,"ws2_32.lib")
    #define  socklen_t (int)
#else
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

#include <pthread.h> 
//
//compile:  g++ server.cpp -o server -lpthread
//

int socket_cmd=0;
int socket_return=0;
char dataSendTotal;
unsigned int ipaddr_int[NUM_CAMARAS_MAX] = {0};
unsigned int num_of_persons[NUM_CAMARAS_MAX] = {0};

unsigned int IPtoInt(char *str_ip) 
{ 
    in_addr addr; 
    unsigned int int_ip;
    if(inet_aton(str_ip,&addr))
    {
        int_ip = ntohl(addr.s_addr);
    }
    return int_ip;
}


void * socket_thread(void *arg)
{
    int i,j=0;
    int running_count = 0;

    //创建套接字
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //将套接字和IP、端口绑定
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  //每个字节都用0填充
    serv_addr.sin_family = AF_INET;  //使用IPv4地址
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  //具体的IP地址
    serv_addr.sin_port = htons(SOCKET_PORT);  //端口
    bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // 设置服务端的socket选项
    #ifndef _WIN32
    int on=1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); 
    #endif

    //进入监听状态，等待用户发起请求
    listen(serv_sock, 1);

    //接收客户端请求
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    int clnt_sock = 0;

    char *recvData = (char*)malloc(MAX_BYTES);
    char *sendData =  (char *)malloc(MAX_BYTES);
    char *ipaddr_string = (char*)malloc(32);

    while(1)
    {
        printf("...listening\n");
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if(clnt_sock < 0 )
        {
            printf("Error: accept\n");
            shutdown(clnt_sock,2);
            close(clnt_sock);
            sleep(1);
            continue;
        }
        printf("one client connected\n");

        while(1)
        {
            // begin recv data
            memset(recvData, 0, sizeof(recvData));
            int len = recv(clnt_sock, recvData, MAX_BYTES, 0);
            if(len <= 0)
            {
                printf("Error: recv communication\n");
                shutdown(clnt_sock,2);
                close(clnt_sock);
                break;
            }
            
			// print all the bytes received
			printf("%d Message form client:\n", len);
			for( i = 0; i < len; i++)
			{
				printf("%02x ",recvData[i]);
				if(',' == recvData[i])
					printf("\n");
			}
			printf("\n");

			// decode the message.
			if(len < 4)
			{
				printf("Server recv %d bytes : %s\n",len,recvData);
				recvData[2] = 0;
				if(strncmp(recvData,"#P",2) == 0) // to capture
				{
                    socket_cmd = 1;
					printf("Start capture pictures!\n");
                    memset(bCameraStatus, 0, NUM_CAMARAS);
                    sprintf(sendData, "JP\n");
                    for(i = 0; i < NUM_CAMARAS; i++)
                    {
                        capture_pic(i);
                       //while(!bCameraStatus[i]);
                    }

                    //if(bCameraStatus[NUM_CAMARAS-1]) // only check the last one
                    if(1)
                    {
                        // shoot done.
                        sendData[0] = 'J';
                        sendData[1] = 'P';
                        sendData[2] = '\n';
                        len = 3;
                    }
                    else
                    {
                        len = 0;
                    }
				}
				else
				{
					len = 0;
					//g_cmd = recvData[1];
				}
			}

            if(len > 0)
            {
                int ret = send(clnt_sock, sendData, len, 0);
                if(ret <= 0)
                {
                    printf("%d Error : send \n", ret);
                    close(clnt_sock);
                    break;
                }
                printf("sent : %s\n",sendData);
            } // end of send
			memset(recvData, 0, sizeof(recvData));
			memset(sendData, 0, sizeof(sendData));
        } // end of <while>
        close(clnt_sock);
    }
    // close thread
    pthread_exit(NULL);
}


