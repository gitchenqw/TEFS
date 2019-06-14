#include <arpa/inet.h>
#include "bserveripPref.h"
#include "DSErrorLog.h"

#define __debug__ 1

void    b_server_set::dump()
{
    printf(".......Bservers alive.......\n");
    
    for(unsigned int i=0;i< MaxServerNum;i++)
    {
        if(inform[i])
        {
            printf("       %s\n",inform[i]->ip);
        }
    }
    printf("...........................\n");
}

void    b_server_set::update_status(char* ip,Task* task,int status)
{
    StrPtrLen fServerIP(ip, strlen(ip));
    OSRef* ref = ServerRefTable.Resolve(&fServerIP);
    if(status == ONLINE)
    {
        if(ref)
        {
            serverip_inform* Sinform = (serverip_inform*)ref->GetObject();
            //if(Sinform->task)
                //Sinform->task->Signal(Task::kKillEvent);
            //Sinform->task = task;
            Sinform->alive++;
            Sinform->live_time = OS::Milliseconds();
#if __debug__
            printf("#BussinessServer(%s),link(open) alive:%d\n",ip,Sinform->alive);
#endif
            ServerRefTable.Release(ref);
        }
        else
        {
            unsigned int i;
            for(i=0;i< MaxServerNum;i++)
            {
                if(!inform[i])
                {
                    inform[i] = new serverip_inform;
                    memcpy(inform[i]->ip,ip,strlen(ip)+1);
                    inform[i]->live_time = OS::Milliseconds();
                    inform[i]->alive++;
                    inform[i]->task = task;
                    inform[i]->fRef.Set(fServerIP,inform[i]);
                    if(ServerRefTable.Register(&inform[i]->fRef) != 0)
                    {
                        inform[i]->task->Signal(Task::kKillEvent);
                        delete inform[i];
                        inform[i] = NULL;
                    }
                    break;
                }
            }
            if(i == MaxServerNum)
                LogError(WarningVerbosity,"BussinessServer<%s> Online failed",ip);
            else
                LogError(MessageVerbosity,"BussinessServer<%s> Online success",ip);
        }
        
    }
    else if(status == HEARTBEAT)
    {
        Assert(ref);
        serverip_inform* Sinform = (serverip_inform*)ref->GetObject();
        Sinform->live_time = OS::Milliseconds();
        ServerRefTable.Release(ref);
#if __debug__
            printf("#BussinessServer(%s),heart beat\n",ip);
#endif
    }
    else if(status == OFFLINE)
    {
        if(ref)
        {
            ServerRefTable.Release(ref);
            serverip_inform* Sinform = (serverip_inform*)ref->GetObject();
            if(Sinform->alive-- <= 0)
                ServerRefTable.UnRegister(ref);
#if __debug__
            printf("#BussinessServer(%s),link(close) alive:%d\n",ip,Sinform->alive);
#endif
        }
    }
    
}
    
char*   b_server_set::GetServerIp()
{ 
    int index;
    int count = 0;
    int i=current;
    while(1)
    {
        index = i%MaxServerNum;
        if(inform[index]) 
        {
            current = index;
            break;
        }
        i++;
        if(count++ > MaxServerNum)
            return NULL;
    }    
    current++;
    return inform[index]->ip;
}

    


int b_server_set::LoadDefaultServer(const char* fname )
{
    FILE    *configFile;
    int     lineCount = 0;
 
    if (!fname) return -1;
    configFile = fopen( fname, "r" );
    Assert( configFile );
    
    if ( configFile )
    {
        SInt32    lineBuffSize = kConfParserMaxLineSize;
        char     lineBuff[kConfParserMaxLineSize];
        char     *next;
      
        do 
        {   
            next = lineBuff;
            
            // get a line ( fgets adds \n+ 0x00 )
            
            if ( fgets( lineBuff, lineBuffSize, configFile ) == NULL )
                break;
            
            lineCount++;

            // trim the leading whitespace
            next = TrimLeft( lineBuff );
                
            if (*next)
            {   
                                
                if ( *next == '#' )
                {
                    // it's a comment
                    //  printf( "comment: %s" , &lineBuff[1] );
                }
                else
                {
                    for(unsigned int i =0;i< MaxServerNum;i++)
                    {
                        if(inform[i] == NULL)
                        {  
                            inform[i] = new serverip_inform;
                            memset(inform[i]->ip,0,sizeof(inform[i]->ip));
                            memcpy(inform[i]->ip,next,strlen(next)-1);
                            StrPtrLen fServerIP(inform[i]->ip,strlen(next)-1);
                            inform[i]->live_time =  OS::Milliseconds();
                            inform[i]->fRef.Set(fServerIP, inform[i]);
                            ServerRefTable.Register(&inform[i]->fRef);
                            break;
                        }
                    }
                    //printf( "serverip: %s" , next );
                }
            }
        
        } while ( !feof( configFile ));
        (void)fclose(  configFile  );
    }
    return 0;
}

void b_server_set::checktimeout()
{
    for(unsigned int i =0;i< MaxServerNum;i++)
    {
        if(inform[i])
        {   
            if( ((OS::Milliseconds()-inform[i]->live_time)/1000 > timeout_interval)||(inform[i]->alive <= 0) )
            {
                ServerRefTable.UnRegister(&inform[i]->fRef);
                if(inform[i]->alive)
                    LogError(WarningVerbosity,"BussinessServer<%s> Offline,timeout",inform[i]->ip);
                else
                    LogError(WarningVerbosity,"BussinessServer<%s> Offline,all link closed",inform[i]->ip);
                delete inform[i];
                inform[i] = NULL;
             }
        }
    }
}

int b_server_set::writetoconf(const char* fname)
{
    FILE    *configFile;
    int     lineCount = 0;
    
    Assert( fname );
    if (!fname) return -1;
    char tempfilename[1024];
    snprintf(tempfilename,1024,"%s.tmp",fname);
    configFile = fopen( tempfilename, "w+" );
    Assert( configFile );
    if ( configFile )
    {
        for(unsigned int i =0;i< MaxServerNum;i++)
        {
            if(inform[i])
            {
                if ( fputs(inform[i]->ip,configFile )<0 || fputc('\n',configFile) < 0)
                {
                    //printf("write error\n");
                    break;
                }
                lineCount++;
            }
        }
        //printf("%d is written\n",lineCount);
        (void)fclose(  configFile  );
    }
    ::remove(fname);
    ::rename(tempfilename,fname);
    return 0;
}


