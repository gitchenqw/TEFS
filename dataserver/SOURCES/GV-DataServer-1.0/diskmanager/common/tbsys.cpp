#include "tbsys.h"
#include <time.h>
#include "string.h"
#include "file_op.h"
#include "func.h"
#define MAXLOGLEN 1024

static std::string logfile("/var/gvfs/dataserver/tfs.log");
static tfs::common::FileOperation* fileop = NULL;
static int printscreen = true;
namespace tfs
{
    int m_LogLevel = TFDEBUG;
    static char* LOGLEVEL[] =
    {
        "DEBUG","INFO","WARN","ERROR"
    };
    
    int TBSYS_LOG(int logLevel, const char* fmt,...)
    { 
        if(NULL == fmt||logLevel < TFDEBUG || logLevel > TFERROR)
        {
            return -1;
        }   
        if(m_LogLevel > logLevel)
        {
            return 0;
        }
        if(!fileop)
            fileop = new common::FileOperation(logfile,O_WRONLY|O_APPEND|O_CREAT);
        char inBuffer[MAXLOGLEN],tempBuffer[MAXLOGLEN];
        va_list args;
        va_start(args,fmt);
        int length = ::vsnprintf(tempBuffer, MAXLOGLEN, fmt, args);
        va_end(args);
        if(length < 0)
        {
            return -1;
        }
        std::string timeformat = tbsys::CTimeUtil::gettimeformat();
        length=snprintf(inBuffer, MAXLOGLEN, "%s <%s> %s\n", timeformat.c_str(),LOGLEVEL[logLevel],tempBuffer);
        if(fileop->write_file(inBuffer,length) < 0)
            printf("write log error,%s\n",strerror(errno));
        if(printscreen)
            printf("%s",inBuffer);
        return 0;
    }
    
    char*itoa64(uint64_t num,char*str,int radix)
    {
        char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        uint64_t unum;
        int i=0,j;
        
        unum=(uint64_t)num;

        do{
            str[i++]=index[unum%(unsigned)radix];
            unum/=radix;
        }while(unum);
        str[i]='\0';
        
        char temp;
        for(j=0;j<=(i-1)/2;j++)
        {
            temp=str[j];
            str[j]=str[i-1-j];
            str[i-1-j]=temp;
        }
        return str;
    }
    
    int atoi64(char *line, size_t n,uint64_t& value)
    {
        if (n == 0) {
            return -1;
        }

        for (value = 0; n--; line++) {
            if (*line < '0' || *line > '9') {
                return -1;
            }

            value = value * 10 + (*line - '0');
        }

        if (value < 0) {
            return -1;

        } else {
            return 0;
        }
    }
    char* combine_itoa(uint64_t fileid,uint32_t chunkid,char* str,int radix)
    {
        char str1[22],str2[11];
        memset(str1,0,sizeof(str1));
        memset(str2,0,sizeof(str2));
        itoa64(fileid,str1,radix);
        itoa64(chunkid,str2,radix);
        str1[strlen(str1)] = ':';
        strcpy(str,str1);
        strcat(str,str2);
        return str;
    }
    namespace tbsys
    {
        void gDeleteA(char* p){gDelete(p);}
        int64_t CTimeUtil::getTime() {return common::Func::curr_time();}
        std::string CTimeUtil::gettimeformat()  
        {
            return common::Func::time_to_str(::time(NULL));
        }
        void CProcess::writePidFile(const char*pid_file){return;}
    }

}