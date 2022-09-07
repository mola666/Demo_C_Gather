#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>

#define PORT 5000

const char str25[] = "UPLOAD IMAGE PATTERN\r\n0\r\n12288054\r\n\r\n";
const char str26[] = "\r\n\r\n";
char buf1[12288155] = {0};
char FPhoto[] = "25601600.bmp";

//读取图像数据传送到 buf1 
int readFile(int PicIndex,char FPhoto[])
{
    //打开图像数据
    FILE *p = fopen(FPhoto, "r"); 
    fseek(p,0,SEEK_END);
    long length=ftell(p);
    fseek(p,0,SEEK_SET);
    //读取图像数据到 buf1 中
    fread(&buf1[PicIndex], 1, length, p);
    fclose(p);
    return PicIndex+length;
}

//将图像数据和命令组合
int concat_str()
{
    buf1[0]='\0';
    //将命令数据发送到 buf1 中
    strcat(buf1,str25);
    //将图像数据发送到 buf1 中
    int StrIndex = readFile(strlen(str25),FPhoto);
    //将 \r\n\r\n 发送到 buf1 中
    for(int i=StrIndex;i<(int)(StrIndex+strlen(str26));i++)
    {
        buf1[i]=str26[i-StrIndex];
    }
    buf1[StrIndex+strlen(str26)] = '\0';
    return StrIndex+strlen(str26)+1;
}

int tcp_send(const char *str,int str_len,bool receive)
{
    char ok[] = {"+OK\r\n\r\n"};
    /*1 创建socket*/
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket failed");
        exit(-1);
    }

    /*2 准备通信地址*/
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    /*设置为服务器进程的端口号*/
    addr.sin_port = htons(PORT);
    /*服务器所在主机IP地址*/
    inet_aton("192.168.0.10", &addr.sin_addr);

    /*3 连接服务器*/
    int res = connect(sockfd,(struct sockaddr *)&addr,sizeof(addr));

    if(res == -1){
        perror("connect failed");
        exit(-1);
    }
    // printf("连接服务器成功....\n");
    /*4 和服务器交换数据*/
    char buf[16] = {0};

    write(sockfd, str, str_len) ;
    if(receive == true)
    {
        read(sockfd, buf, sizeof(buf));
        if(strcmp(buf,ok)) {
            printf("命令输入失败！\r\n");
        }else{
            printf("%s\r\n",buf);
        }
    }
    /*关闭连接*/
    close(sockfd);
    return 0;
}

void dlp_init()
{
    const char str1[] = "\r\n\r\n";
    const char str2[] = "SET OPERATION MODE PATTERN_ON_THE_FLY_MODE\r\n\r\n";
    const char str3[] = "SET LUT DEFINITION 33100,0,1,8,0,0,0\r\n\r\n";
    const char str4[] = "SET LUT CONFIG 1 0\r\n\r\n";
    const char str5[] = "SET AMPLITUDE 100\r\n\r\n";
    const char str6[] = "SET OCP 24\r\n\r\n";
    const char str7[] = "SET REG MODE COMBINED\r\n\r\n";
    const char str8[] = "SET BOARD TEMP LIMIT 60\r\n\r\n";
    const char str9[] = "SET LED TEMP LIMIT 40\r\n\r\n";
    const char str10[] = "SET SEQ ON\r\n\r\n";
    const char str11[] = "SET LIGHT ON\r\n\r\n";
    

    int StrSize = concat_str();

    tcp_send(str2,sizeof(str2),true);
    tcp_send(str3,sizeof(str3),true);
    tcp_send(str4,sizeof(str4),true);
    tcp_send(buf1,StrSize,true);
    tcp_send(str5,sizeof(str5),true);
    tcp_send(str6,sizeof(str6),true);
    tcp_send(str7,sizeof(str7),true);
    tcp_send(str8,sizeof(str8),true);
    tcp_send(str9,sizeof(str9),true);
    tcp_send(str10,sizeof(str10),true);
    tcp_send(str11,sizeof(str11),true);
}

void dlp_on()
{
    const char str10[] = "SET SEQ ON\r\n\r\n";
    const char str11[] = "SET LIGHT ON\r\n\r\n";
    tcp_send(str10,sizeof(str10),true);
    tcp_send(str11,sizeof(str11),true);
}

void dlp_off()
{
    const char str10[] = "SET SEQ OFF\r\n\r\n";
    const char str11[] = "SET LIGHT OFF\r\n\r\n";
    tcp_send(str11,sizeof(str11),true);
    tcp_send(str10,sizeof(str10),true);
}



int main()
{
    dlp_init();
    int a=200;
    while(true)
    {
        dlp_on();
        sleep(10);
        dlp_off();
        sleep(10);
        a--;
        printf("a=%d\r\n",a);
        if(a==0) break;
    }

    printf("exit!");
    return 0;
}