#ifndef _H_TBSYS
#define _H_TBSYS
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <sstream> 

namespace tfs
{
    enum
    {
        TFDEBUG,TFINFO,TFWARN,TFERROR
    };
    int TBSYS_LOG(int logLevel, const char* fmt,...);
    char*itoa64(uint64_t num,char*str,int radix);
    int atoi64(char *line, size_t n,uint64_t& value);
    char* combine_itoa(uint64_t fileid,uint32_t chunkid,char* str,int radix);
    
    namespace tbsys
    {
        template <typename T>
        void gDelete(T& p)
        {
            if(p)
            { 
                delete p;
                p=NULL;
            }
        }
        void gDeleteA(char* p);
        class CTimeUtil
        {
        public:
            static int64_t getTime();
            static std::string gettimeformat(); 
        };
        class CProcess
        {
        public:
           static void writePidFile(const char*pid_file);
        };
    }
}
#endif