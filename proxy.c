
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#define BUFFSIZE 512
#define PORTI 21
char *cmd_port;
int i;
int portnum = 25000;
int isActive=1;
int isDownloading=0;
int isDownloadingCache=0;
int isSending=0;
int haveCache=0;
//char buffer[BUFFSIZE];
char *filename;
int fd;

//bind and listen for tcp connection
int bindAndListenSocket(int port)
{
    int socketfd;
    struct sockaddr_in my_addr;
    if( (socketfd=socket(AF_INET, SOCK_STREAM, 0))==-1)
    {
        perror("proxy cmd socket failed");
        exit(1);
    }
    printf("\nsocket = %d\n",socketfd);
    my_addr.sin_family=AF_INET;
    my_addr.sin_port=htons(port);
    my_addr.sin_addr.s_addr=INADDR_ANY;
    bzero(&(my_addr.sin_zero),8);
    if(bind(socketfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))==-1)
    {
        perror("bind出错！");
        exit(1);
    }
    if(listen(socketfd,10)==-1)
    {
        perror("listen 出错！");
        exit(1);
    }
    printf("bind success port : %d",port);
    return socketfd;
}

//Transform port number into string
char* getPortnum(int port)
{
    int a = port/256;
    int b = port % 256;
    char *portnum;
    portnum=(char *)malloc(10);
    memset(portnum, 0, sizeof(portnum));
    sprintf(portnum, "%d,%d",a,b);
    printf("\n%s\n",portnum);
    return portnum;
}

//accept function for tcp connection
int acceptSocket(int sock_in)
{
    struct sockaddr_in remote_addr;
    socklen_t sin_size;
    int socketfd;
    sin_size=sizeof(struct sockaddr_in);
    if((socketfd=accept(sock_in, (struct sockaddr*)&remote_addr, &sin_size))==-1)
    {
        printf("accept error %d",sock_in);
        exit(1);
    }
    printf("\nacceptsocket = %d\n",socketfd);
    printf("accept success");
    return socketfd;
}

//connect function used in command connection
int connectToServer()
{
    int socketfd;
    struct sockaddr_in serv_addr;
    if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket create error!");
        exit(1);
    }
    printf("\nserversocket = %d\n",socketfd);
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(21);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.56.1");
    bzero(&(serv_addr.sin_zero),8);
    if(connect(socketfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr))==-1){
        perror("connect error1!");
        exit(1);
    }
    printf("connect success");
    
    return socketfd;
}

//connect function in data connection
int connectToClient()
{
    int port;
    char *temp;
    int num1 = 0,num2 = 0;
    temp = strtok(cmd_port, ",");
    for(i=1;i<6;i++)
    {
        temp=strtok(NULL,",");
        if(i==4)
        {
            num1 = atoi(temp);
        }
        if(i==5)
        {
            num2 = atoi(temp);
        }
    }
    port = 256*num1 + num2;
    printf("\nport calculated%d\n",port);
    int socketfd;
    struct sockaddr_in serv_addr;
    
    
    if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("socket create error!");
        exit(1);
    }
    printf("\nclientsocket = %d\n",socketfd);
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.56.1");
    bzero(&(serv_addr.sin_zero),8);
    if(connect(socketfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr))==-1){
        perror("connect error2!");
        exit(1);
    }
    printf("connect success2");
    
    return socketfd;
}
int main(int argc, const char *argv[])
{
    fd_set master_set, working_set;
    struct timeval timeout;
    int proxy_cmd_socket    = 0;
    int accept_cmd_socket   = 0;
    int connect_cmd_socket  = 0;
    int proxy_data_socket   = 0;
    int accept_data_socket  = 0;
    int connect_data_socket = 0;
    int selectResult = 0;
    int select_sd = 50;
    int recvBytes=0;

    
    FD_ZERO(&master_set);
    bzero(&timeout, sizeof(timeout));
    
    proxy_cmd_socket = bindAndListenSocket(21);

    FD_SET(proxy_cmd_socket, &master_set);
    
    timeout.tv_sec = 600;
    timeout.tv_usec = 0;
    while (1) {
        //set working_set
        FD_ZERO(&working_set);
        memcpy(&working_set, &master_set, sizeof(master_set));
        
        
        selectResult = select(select_sd, &working_set, NULL, NULL, &timeout);
        
        
        if (selectResult < 0) {
            perror("select() failed\n");
            exit(1);
        }
        
       
        if (selectResult == 0) {
            printf("select() timed out.\n");
            timeout.tv_sec=600;
            continue;
        }
        
        
        for (i = 0; i < select_sd; i++) {
                        if (FD_ISSET(i, &working_set)) {
                
                //command connection establishing
                if (i == proxy_cmd_socket) {
                    accept_cmd_socket = acceptSocket(proxy_cmd_socket);
                    connect_cmd_socket = connectToServer();
                    
                  
                    FD_SET(accept_cmd_socket, &master_set);
                    FD_SET(connect_cmd_socket, &master_set);
                }
                
                if (i == accept_cmd_socket) {
                    char buff[BUFFSIZE] = {0};
                    
                    if (read(i, buff, BUFFSIZE) == 0) {
                        close(i);
                        close(connect_cmd_socket);
                        
                        
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);
                        
                    } else {

                        
                        //PORT MODE and processing the port command with proxy port number
                        if(buff[0]=='P'&&buff[1]=='O'&&buff[2]=='R'&&buff[3]=='T')
                        {       isActive=1;
                                printf("\nActive Mode\n");
                                proxy_data_socket = bindAndListenSocket(portnum);
                                FD_SET(proxy_data_socket, &master_set);
                                printf("\nproxydatasocket = %d\n",proxy_data_socket);
                                char *temp;
                                temp=(char *)malloc(100);//
                                memset(temp, 0, sizeof(temp));
                                temp=strtok(buff, " ");
                                temp=strtok(NULL, " ");
                                //split(temp, buff, " ");
                                //strcpy(cmd_port, temp[1]);
                                cmd_port=(char *)malloc(50);//
                                memset(cmd_port, 0, sizeof(cmd_port));
                                strcpy(cmd_port, temp);
                                printf("splited %s",temp);
                                char *port;
                                port=(char *)malloc(100);
                                memset(port, 0, sizeof(port));
                                sprintf(port, "PORT 192,168,56,101,%s\r\n",getPortnum(portnum));
                                portnum++;
                                printf("sent port is %s",port);
                                write(connect_cmd_socket, port, strlen(port));
                            }
                        
                        //RETR command and check if have cache
                        else if(buff[0]=='R'&&buff[1]=='E'&&buff[2]=='T'&&buff[3]=='R')
                        {
                            isDownloading=1;
                            char *temp;
                            char *command;
                            command=(char *)malloc(100);//
                            memset(command, 0, sizeof(command));
                            sprintf(command,"%s",buff);
                            temp=strtok(buff, " ");
                            temp=strtok(NULL, " ");
                            filename=strtok(temp, "\r");
                            printf("\nfilename = %s\n",filename);
                            printf("\ncommand = %s\n",command);
                            if((fd=open(filename,O_RDONLY))==-1)
                            {
                                haveCache=1;
                                printf("no cache");
                                fd=open(filename, O_RDWR|O_CREAT);
                                write(connect_cmd_socket, command, strlen(command));

                                //memset(buffer, 0, sizeof(buffer));
                            }else{
                                printf("have cache");
                                haveCache=2;
                                write(connect_cmd_socket, command, strlen(command));

                            }
                            printf("%d",fd);
                        }
   
                        
                            else{
                                int sentlength=write(connect_cmd_socket, buff, strlen(buff));
                                printf("sent length is %d\n",sentlength);
                                
                            }

                      
                    }
                }
                
                if (i == connect_cmd_socket) {
                    char buff[BUFFSIZE]={0};
                    
                    if((recvBytes=read(i, buff, BUFFSIZE))!=0)
                    {
                        //PASV MODE send 227 command with proxy port number
                        if(buff[0]=='2'&&buff[1]=='2'&&buff[2]=='7')
                        {   isActive=0;
                            proxy_data_socket = bindAndListenSocket(portnum);
                            FD_SET(proxy_data_socket, &master_set);
                            printf("\npassive port command : %s\n",buff);
                            char *temp;
                            //temp=(char *)malloc(50);
                            //memset(temp, 0, sizeof(temp));
                            temp=strtok(buff, "(");
                            temp=strtok(NULL, "(");
                            cmd_port=strtok(temp, ")");
                            printf("\ntemp = %s\n",temp);
                            cmd_port=(char *)malloc(50);//
                            memset(cmd_port, 0, sizeof(cmd_port));
                            strcpy(cmd_port, temp);
                            char *port;
                            port=(char *)malloc(50);
                            memset(port, 0, sizeof(port));
                            sprintf(port, "227 Entering Passive Mode (192,168,56,101,%s)\r\n",getPortnum(portnum));
                            write(accept_cmd_socket,port,strlen(port));
                            portnum++;

                        }
                        
                        //transfer complete
                        else if(buff[0]=='2'&&buff[1]=='2'&&buff[2]=='6'&&isDownloading==1)
                        {
                            isDownloading=0;
                            close(fd);
                            haveCache=0;
                            write(accept_cmd_socket, buff, strlen(buff));
                        }

                        else{
                            printf("connect_cmd_socket recvBytes is %d",recvBytes);
                            write(accept_cmd_socket, buff, strlen(buff));
                        }
                        
                    }
                    else{
                        close(accept_cmd_socket);
                        close(connect_cmd_socket);
                        
                        
                        FD_CLR(accept_cmd_socket, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);

                    }

                  
                }
                
                if (i == proxy_data_socket) {
                    accept_data_socket=acceptSocket(proxy_data_socket);
                    FD_SET(accept_data_socket, &master_set);
                    connect_data_socket=connectToClient();
                    FD_SET(connect_data_socket,&master_set);
                    close(proxy_data_socket);
                    FD_CLR(proxy_data_socket, &master_set);
                                   }
                
                if (i == accept_data_socket) {
                    char buff[BUFFSIZE] = {0};
                    if (((recvBytes=read(i, buff, BUFFSIZE)) == 0)&&(isDownloading==0)) {
                        close(accept_data_socket);
                        close(connect_data_socket);
                        
                        
                        FD_CLR(accept_data_socket, &master_set);
                        FD_CLR(connect_data_socket, &master_set);
                    }
                    else{
                        
                        //no cache
                        if(haveCache==1)
                        {
                            write(connect_data_socket, buff, recvBytes);
                            write(fd, buff, recvBytes);
                        }
                        
                        //have cache
                    else if(haveCache==2)
                        {   char buf[BUFFSIZE]={0};
                            while ((recvBytes=read(fd,buf,BUFFSIZE))>0)
                             {
                                printf("read length = %d\n",recvBytes);
                                write(connect_data_socket, buf, BUFFSIZE);
                            }
                        }
                        
                        //other data request
                        else if(isDownloading==0)
                        {
                            write(connect_data_socket, buff, recvBytes);
                        }
                    }
                                  }
                
                if (i == connect_data_socket) {
                    char buff[BUFFSIZE] = {0};
                    if (((recvBytes=read(i, buff, BUFFSIZE)) == 0)&&(isDownloading==0)) {
                        close(accept_data_socket);
                        close(connect_data_socket);
                        
                       
                        FD_CLR(accept_data_socket, &master_set);
                        FD_CLR(connect_data_socket, &master_set);
                    }
                    else{
                        //no cache
                        if(haveCache==1)
                        {
                            write(accept_data_socket, buff, recvBytes);
                            write(fd, buff,recvBytes);
                        }else if(haveCache==2)
                            //have cache
                        {   char buf[BUFFSIZE]={0};
                            while((recvBytes=read(fd, buf, BUFFSIZE))>0)
                            {
                                printf("read length = %d\n",recvBytes);
                                write(accept_data_socket, buf, BUFFSIZE);
                            }
                        }
                        
                        //other request
                        else if(isDownloading==0)
                        {
                            write(accept_data_socket, buff, recvBytes);
                        }
                    }
                   
                }
            }
        }
    }
    
    return 0;
}
