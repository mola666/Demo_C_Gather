#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    int fd,ret;

    //1、创建一个100个字节大小的数组
    char buf[100] = {0};  
    mode_t mode = 0777;

    //2、打开文件，并以字节的形式读取文件中的内容
    fd = open("./temp.txt", O_RDWR | O_APPEND | O_CREAT , mode);
    ret = read(fd, buf, 100);
    if(ret == -1) printf("read faile!\r\n");
    printf("buf is: \r\n%s\r\n",buf);

    //3、将 buf 字符串，以 逗号（,） 分割开来。并将分割开来的字符串转化为 数字
    char *token=strtok(buf,",");
    int a=0;
    int b=0;
    while(token!=NULL){
        // printf("%s\n\r",token);
        a = atoi(token);
        b = a + b;
        printf("%d\n",a);
        token=strtok(NULL,",");
    }

    //4、验证是否转化为了数字
    printf("%d\n",b);

    printf("run exit!\r\n");
    return 0;
}


