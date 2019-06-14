/*manage Bussiness Server
 * */
#ifndef __BSERVERIPPREF_H__
#define __BSERVERIPPREF_H__

#include <stdio.h> 
#include "OSRef.h"
#include "OS.h"
#include "Task.h"
#include "Trim.h"


class b_server_set
{
public: 
    enum
    {
        kConfParserMaxLineSize = 1024,
        MaxServerNum           = 1024
    };
    enum
    {
        ONLINE,
        HEARTBEAT,
        OFFLINE
    };
    b_server_set()
    {
        memset(inform,0,sizeof(inform));
        current = 0;
    }
    ~b_server_set()
    {
        for(unsigned int i=0;i< MaxServerNum;i++)
            if(inform[i]) delete inform[i];
    }
    struct serverip_inform
    {
        OSRef            fRef;
        char            ip[64];
        SInt64           live_time;//ms
        Task*            task;
        int              alive;
        serverip_inform():task(NULL),alive(0){}
    };
    void    dump();
    void    SetTimeout(SInt64 timeout){timeout_interval = timeout;}   
    int     LoadDefaultServer(const char* fname );
    void    update_status(char* ip,Task*task,int status = HEARTBEAT);
    void    checktimeout();
    char*   GetServerIp();
    int     writetoconf(const char* fname);
    
private:
    OSRefTable           ServerRefTable;
    serverip_inform*     inform[MaxServerNum];
    int                  current;
    SInt64               timeout_interval;
};
#endif


