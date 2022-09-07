#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>


// #include <stdarg.h>
// #include <stdio.h>
// #include <string.h>
// #include <time.h>
// #include <unistd.h>
// #include <sys/time.h>
// #include <sys/stat.h>
// #include <pthread.h>
// #include <sys/msg.h>
// #include <sys/ipc.h>
// #include <errno.h>
// #include <dirent.h>
// #include <stdlib.h>

#define LOGFILE_PATH "./log"
//日志文件名称的前缀
#define LOGFILE_PREFIX "log_"//"log_b503_"

#define DEVICE "BASKE2.0" 

//目前暂只支持管理目录下的30个日志文件名，文件名长度50以内。大了不处理
#define LOGFILE_MAXCOUNT  7300
#define LOGFILE_NAMELENTH 50
static char file_names[LOGFILE_MAXCOUNT][LOGFILE_NAMELENTH];

//记录文件名前缀（最好取自终端编号）
static char file_prifix[LOGFILE_NAMELENTH];

//日志文件存在的时间 单位（天）,会自动删除当前日期-ALIVEDAYS 之前的文件
//限制日志最长保留时间不能超 LOGFILE_MAXCOUNT 天
#define LOGFILE_ALIVEDAYS 7300  

typedef __key_t key_t;

//linux消息队列
static int s_msg_id;
static int r_msg_id;

static pthread_t tid;

enum LogLevel
{
    ERROR = 1,
    WARN  = 2,
    INFO  = 3,
    DEBUG = 4,
};

//单个日志文件的限制大小 单位kb
//#define LOGFILE_MAXSIZE 1*1024 
#define LOGFILE_MAXSIZE 10*1024*1024 //10M

// 使用了GNU C扩展语法，只在gcc（C语言）生效，
// g++的c++版本编译不通过
static const char* s_loginfo[] = {
    [ERROR] = "ERROR",
    [WARN]  = "WARN",
    [INFO]  = "INFO",
    [DEBUG] = "DEBUG",
};

void mylog1(const char* filename, int line, enum LogLevel level, const char* fmt, ...) __attribute__((format(printf,4,5)));
 
#define mylog(level, format, ...) mylog1(__FILE__, __LINE__, level, format, ## __VA_ARGS__)


#define MSG_TYPE 1001
 
#define MAX_TEXT 1024
 
struct msg_st{
	long int msg_type;
	char text[MAX_TEXT];
};

#define LOGLEVEL DEBUG

//创建文件夹
#if 1
static int create_dir(const char *sPathName)  
{  
	char dirName[256];  
	strcpy(dirName, sPathName);  
	int len = strlen(dirName);  //查看字符串的长度
    printf("string long %d\n",len);
	if(access(dirName, 0)!=0)  //检测到目录存在则返回0,否则返回 -1
	{  
		if(mkdir(dirName, 0755)==-1)  //创建目录，权限为0755 。如果返回值为-1,则返回下述错误
		{   
			fprintf(stderr,"mkdir   error\n");   
			return -1;   
		}  
	}  
	return 0;  
} 
#endif 


//读取目录中的文件列表
#if 1
static int read_filelist(char *basePath)
{
	DIR *dir;
	struct dirent *ptr;

	if ((dir=opendir(basePath)) == NULL)  //1、打开该目录 。如果打开失败，则返回 NULL
	{
		fprintf(stderr,"Open dir error...");
		return -1;
	}

	while ((ptr=readdir(dir)) != NULL)  //2、成功则返回下个目录进入点。有错误发生或读取到目录文件尾则返回NULL.
	{
		printf("d_name : %s\n", ptr->d_name);
	}
	closedir(dir);
	return 0;
}
#endif 

#if 1
//获取当前时间
static int get_curtime( char* outTimeStr )
{
	int ret = 0;
	time_t tTime;
	struct tm *tmTime;
	time( &tTime );  //获取时间
	tmTime = localtime( &tTime );  //获取本地的时区

    //1、第一个参数 outTimeStr 是接收字符串的字符数组
    //2、第二个参数就是要传的字符串
	sprintf( outTimeStr, "%04d%02d%02d%02d%02d%02d",  
			tmTime->tm_year + 1900, tmTime->tm_mon + 1,
			tmTime->tm_mday, tmTime->tm_hour,
			tmTime->tm_min, tmTime->tm_sec );
	return ret;
}
#endif


#if 1
//处理日志文件是否保留
//原理算法：把日期转换成时间戳，然后由配置的允许保留的天数换算出一个时间范围，
//在遍历日志目录中所有的文件名，提取出日期，在这个时间范围内的保留，否则删除
//关键的地方，算出这个允许保留文件的时间范围，原理是日期转时间戳，再时间戳转日期  
static int file_alives_proc()
{
	int ret = 0;
	char curtime[20];  //当前日期时间
	char deadtime[20]; //截止的历史日期
 
	printf("file_alives_proc:\n");
	get_curtime(curtime);
	//只取日期，省去时间
	memset(&curtime[8],0x30,6);
	printf("ailve maxdays:%d\n",LOGFILE_ALIVEDAYS);  //打印输出，日志可保存的最长天数
	printf("curtime:%s\n",curtime);  //打印当前的日期
	
	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));  //申请一块内存
	//字符串转时间
	strptime(curtime,"%Y%m%d%H%M%S",tmp_time);
	time_t t = mktime(tmp_time);
	printf("t now = %ld\n",t);
 
	free(tmp_time);  
 
	time_t t1 = t - LOGFILE_ALIVEDAYS*24*60*60;
	//再把t1转换为时间，即时间戳转时间
	
	struct tm *p; 
	p = gmtime(&t1); 
	//日期时间转字符串,由于只比较日期，因此忽略时间
	strftime(deadtime, sizeof(deadtime), "%Y%m%d000000", p); 	
	printf("deadtime:%s\n",deadtime);
 
	//以上获取到了curtime和deadtime,有了这个时间范围，接下来就去找这个范围的日志
	//日志文件日期在这个范围内的保留，否则删除
	for(int i = 0; i < LOGFILE_MAXCOUNT; i++ )
	{
		if(strlen(file_names[i]) > 0)
		{
			printf("file_name=%s\n",file_names[i]);
			char ftime[20];
			memset(ftime,0,20);
			memset(ftime,0x30,8);
			//关键，这个截取不能错
			memcpy(ftime,&file_names[i][strlen(LOGFILE_PREFIX)+1+strlen(file_prifix)],8);
			printf("file_time=%s\n",ftime);
			
			//开始比较 是否在日期范围内
			if(memcmp(ftime,deadtime,8) > 0)
			{
				//大于截止日期的文件保留
				printf("%s------keep alive\n",file_names[i]);
			}else{
				printf("%s----------------dead,need remove!\n",file_names[i]);
				//删除文件
				char dfname[50];
				sprintf(dfname,"%s/%s",LOGFILE_PATH,file_names[i]);
				remove(dfname);
			}
			//
		}else{
			//printf("fname=NULL\n");
		}
 
	}
 
	return ret;
}
#endif

#if 1
int open_msg(void) {
	key_t msgkey;
	
	// if ((msgkey = ftok("./tmp", 'a')) < 0) {
	// 	printf("send ftok failed!\n");
	// 	return -1;
	// }

	if ((msgkey = ftok(LOGFILE_PATH , 'a')) < 0) {
		printf("send ftok failed!\n");
		return -1;
	}

	printf("----msgkey is %d\n",msgkey);  //输出文档索引节点

	if ((s_msg_id = msgget(msgkey, IPC_CREAT | 0666)) == -1) {
		printf("msgget failed!\n");
		return -1;
	}

	printf("----s_msg_id is %d\n",s_msg_id);
	msgctl(s_msg_id,IPC_RMID,0);//先删除，否则可能满，因其生命周期同内核

	if ((s_msg_id = msgget(msgkey, IPC_CREAT | 0666)) == -1) {    //创建新的消息队列标识符
		printf("msgget failed!\n");
		return -1;
	}

	r_msg_id = s_msg_id;
	printf("----r_msg_id is %d\n",r_msg_id);

	return 0;
}
#endif

#if 1
//获取文件大小
static unsigned long get_size(const char *path)
{
	unsigned long filesize = -1;	
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}
#endif

#if 1
//若大量连续发的太快，收的太慢，会导致发送失败
static int recv_msg(void)
{
	int rsize;
	struct msg_st data;  //创建一个 data 的数据结构
	int msgtype = MSG_TYPE;
 
	char tmpfname[ 50 ];
	char tmpTime[ 14 + 1 ];
	FILE* fp;
	unsigned long filesize;
 
	memset(data.text,0,sizeof(data.text));  //将一段连续的内存初始化为某个值
	/*接收消息队列*/
	//阻塞接收

	/*
	r_msg_id: 由消息队列的标识符

	ptr :消息缓冲区指针。消息缓冲区结构为：
	struct msgbuf {
	long mtype;
	char mtext[1];
	｝

	length: 消息数据的长度

	type: 决定从队列中返回哪条消息：
	=0 返回消息队列中第一条消息
	>0 返回消息队列中等于mtype 类型的第一条消息。
	<0 返回mtype<=type 绝对值最小值的第一条消息。

	msgflg 为０表示阻塞方式，设置IPC_NOWAIT 表示非阻塞方式

	msgrcv 调用成功返回0，不成功返回-1。
	*/
	rsize = msgrcv(r_msg_id,&data,sizeof(data.text),msgtype,MSG_NOERROR );    //从消息队列中读出一条新消息
	if(rsize == -1)
	{
		if (errno != ENOMSG) {
			perror("msgrcv");
		}
		printf("No message available for msgrcv()\n");
		return -1;
	}
	else
	{
		//printf("message received: %s\n", data.text);
		get_curtime( tmpTime );

		//拼接字符串为文件的名称
		sprintf( tmpfname, "%s/%s%s_%8.8s.log", LOGFILE_PATH, LOGFILE_PREFIX,file_prifix, tmpTime );
		fp = fopen( tmpfname, "a" );  //打开文件 tmpfname ，以 a 的方式，打开或新建一个文本文件，只允许在文件末尾追写
		if ( NULL == fp )
		{
			fprintf(stderr,"failed to open file,filename=%s\n",tmpfname);
			return -2;
		}
		else
		{
			filesize = get_size(tmpfname);
		//	printf("filesize=%u\n",filesize);
			if((filesize/1024) > LOGFILE_MAXSIZE){
				fprintf(stderr,"failed to write log,only allow maxsize=%ukb,current size=%ukb\n",LOGFILE_MAXSIZE,filesize/1024);
				fclose(fp);
				return -3 ;
			}
			//printf("%s\n",tmpfname);
			fprintf( fp, "%s", data.text );
			fclose(fp);
		}
	}
	return 0;
}
#endif

#if 1
//考虑了一点儿效率，操作IO比较耗时，那就异步写入吧
//想用单独的线程，负责写日志到文件,自己实现消息队列?
//不这么做了，直接使用linux的消息队列吧
static void* thread_writelog(void* args)
{
	mylog(INFO,"thread_writelog begin...");//\n
	while(1)
	{
		recv_msg();
		usleep(20*1000);							
	}
}
#endif

#if 1
//=============================================
static void get_timestamp(char *buffer)
{
    time_t t;
    struct tm *p;
    struct timeval tv;
    int len;
    int millsec;
 
    t = time(NULL);  //返回当前时间
    p = localtime(&t);  //本地时间
 
    gettimeofday(&tv, NULL);  //获取当前时间和时区信息
    millsec = (int)(tv.tv_usec / 1000);
 
    /* 时间格式：[2011-11-15 12:47:34:888] */
    // len = snprintf(buffer, 32, "[%04d-%02d-%02d %02d:%02d:%02d:%03d] ",
        // p->tm_year+1900, p->tm_mon+1,
        // p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, millsec);
     len = snprintf(buffer, 21, "[%02d:%02d:%02d:%03d] ",  //用于将格式化的数据写入字符串
        p->tm_hour, p->tm_min, p->tm_sec, millsec);
    buffer[len] = '\0';
}
#endif

#if 1
//记录日志到文件
static int send_msg(char *buf,int len){
	struct msg_st data;
	data.msg_type = MSG_TYPE;
	strcpy(data.text,buf);
	//s_msg_id = 0;
	if(msgsnd(s_msg_id,&data,len,IPC_NOWAIT) == -1){
		printf("msgsnd failed.\n");
		perror("msgsnd");
		return -1;
	}
	return 0;
}
#endif

#if 1
void mylog1(const char* filename, int line, enum LogLevel level, const char* fmt, ...)
{
    if(level > LOGLEVEL)
        return;
 
    va_list arg_list;
    char buf[1024];
	char sbuf[MAX_TEXT];
 
    memset(buf, 0, 1024);
	memset(sbuf,0,MAX_TEXT);
 
    va_start(arg_list, fmt);  //用于获取函数参数列表中可变参数的首指针(获取函数可变参数列表)
    vsnprintf(buf, 1024, fmt, arg_list);  //向一个字符串缓冲区打印格式化字符串，且可以限定打印的格式化字符串的最大长度。
    char time[32] = {0};
 
    // 去掉*可能*存在的目录路径，只保留文件名
    const char* tmp = strrchr(filename, '/');  //该函数返回 filename 中最后一次出现字符 / 的位置。如果未找到该值，则函数返回一个空指针。
    if (!tmp) tmp = filename;
    else tmp++;
    get_timestamp(time);
 
	switch(level){
		case DEBUG:
			//绿色
			sprintf(sbuf,"%s[%s] [%s:%d] %s\n", time, s_loginfo[level], tmp, line, buf);
			break;
		case INFO:
			//蓝色
			sprintf(sbuf,"%s[%s] [%s:%d] %s\n", time, s_loginfo[level], tmp, line, buf);
			break;
		case ERROR:
			//红色
			sprintf(sbuf,"%s[%s] [%s:%d] %s\n", time, s_loginfo[level], tmp, line, buf);
			break;
		case WARN:
			//黄色
			sprintf(sbuf,"%s[%s] [%s:%d] %s\n", time, s_loginfo[level], tmp, line, buf);
			break;
	}
 
	printf("%s",sbuf);

	//记录日志到文件
	send_msg(sbuf,strlen(sbuf));

    va_end(arg_list);  //置空 arg_list , 即 arg_list = 0 ;
}
#endif

int main()
{
    int ret=0;
    printf("init log:\n");
    //1、创建目录
    ret = create_dir(LOGFILE_PATH);
	if(0 == ret){
		printf("create success!\n");
	}else {
        return ret;
    }
    
    //2、读取目录中的文件列表
    ret = read_filelist(LOGFILE_PATH);
    if(0 != ret){
		printf("read_filelist err!,ret =%d\n",ret);
	}

    //3、给文件名拷贝到 file_prifix
	if(strlen(DEVICE) > 0){
		strcpy(file_prifix,DEVICE);
	}

    //4、处理是否保留历史记录文件,暂时可不用
	// file_alives_proc();

	//5、创建消息队列
	ret = open_msg();
	if(0 != ret){
		return ret;
	}

	#if 1
	//6、创建写日志文件线程
	ret = pthread_create(&tid,NULL,thread_writelog,NULL);
	if(0 != ret)
	{
		fprintf(stderr, "couldn't create thread_writelog, errno %d\n", ret);
	}
	else
	{
		printf("create thread_writelog success!\n");
	}
	printf("init log success!enabled log file...\n");
	#endif

	//7、使用日志文件函数
	mylog(INFO,"Print_Fun step 1 over");

	while(1){};

    return 0;

}