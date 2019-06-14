
#ifndef __ADDRERRORLOG_H__
#define __ADDRERRORLOG_H__


#include "OSMemory.h"
#include "OS.h"
#include "Task.h"
#include "DSHeader.h"
#include "QTSSRollingLog.h"
#include "DSServerPref.h"
#include "defaultpref.h"

enum
{
MAXLOGNAMELEN         = 128,
MAXLOGDIRLEN          = 256,
MAXLOGLEN             = 1024,
MaxTimeBufferSize     = 30 
};


extern ServerPref* svrPref;

int   LogError(char* inBuffer);
int   LogError(UInt32 inVerbosity,char* inBuffer);
int   LogError(UInt32 inVerbosity,const char* fmt,...);


class ADDRErrorLog : public QTSSRollingLog
{
public:
    
    ADDRErrorLog() : QTSSRollingLog() {this->SetTaskName("ADDRErrorLog");}
    virtual ~ADDRErrorLog() {}
    
protected:
    virtual char* GetLogName()  
    { 
        char* name = new char[MAXLOGNAMELEN];
        if(svrPref->GetErrorLogName())
            memcpy(name,svrPref->GetErrorLogName(),MAXLOGNAMELEN);
        else
            memcpy(name,DEFAULTLOG_NAME,sizeof(DEFAULTLOG_NAME));
        return name;
    }
        
    virtual char* GetLogDir()  
    { 
        char* dir = new char[MAXLOGDIRLEN];
        if(svrPref->GetErrorLogDir())
            memcpy(dir,svrPref->GetErrorLogDir(),MAXLOGDIRLEN);
        else
            memcpy(dir,DEFAULTLOG_DIR,sizeof(DEFAULTLOG_DIR));
        return dir;
    }
        
    virtual UInt32 GetRollIntervalInDays()  { return svrPref->GetRollIntervalInDays();}
        
    virtual UInt32 GetMaxLogBytes() { return svrPref->GetMaxErrorLogBytes();}
    
    /*virtual time_t WriteLogHeader(FILE* inFile)
    {
        QTSSRollingLog::WriteLogHeader(inFile);
        char tempBuffer[MAXLOGLEN];
        snprintf(tempBuffer,MAXLOGLEN,"#Software: %s\n#Version: %s",Software,Version);
        this->WriteToLog(tempBuffer, !kAllowLogToRoll);
    }*/
        
private:
    ADDR_ErrorVerbosity ADDR_Verbosity;
    
};

//ERRORLOGCHECKTASK CLASS DEFINITION
class ErrorLogCheckTask : public Task
{
    public:
        ErrorLogCheckTask() : Task() {this->SetTaskName("ErrorLogCheckTask"); this->Signal(Task::kStartEvent); }
        virtual ~ErrorLogCheckTask() {}
        
    private:
        virtual SInt64 Run();
};

#endif


