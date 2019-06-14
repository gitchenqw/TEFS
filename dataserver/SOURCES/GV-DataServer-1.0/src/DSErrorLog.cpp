#include <stdarg.h>
#include "DSErrorLog.h"


typedef char* LevelMsg;
static LevelMsg sErrorLevel[] = {
	": FATAL :",
	": WARNING :",
	": INFO :",
	": ASSERT :",
	": DEBUG :"
};

static ADDRErrorLog*      sErrorLog = NULL;
static void CheckErrorLogState();
// This task runs once an hour to check and see if the log needs to roll.
SInt64 ErrorLogCheckTask::Run()
{
    static Bool16 firstTime = true;
    
    // don't check the log for rolling the first time we run.
    if (firstTime)
    {
        firstTime = false;
    }
    else
    {
        Bool16 success = false;

        if (sErrorLog != NULL && sErrorLog->IsLogEnabled())
            success = sErrorLog->CheckRollLog();
        Assert(success);
    }
    // execute this task again in one hour.
    return (60*60*1000);
}

int LogError(char* inBuffer)
{
    if(!inBuffer) return -1;
    CheckErrorLogState();
    if (sErrorLog == NULL)
        return 0;
    char tempBuffer[MAXLOGLEN];
    char timeBuffer[MaxTimeBufferSize];
    if(!QTSSRollingLog::FormatDate(timeBuffer,false))
        return -1;
    snprintf(tempBuffer,MAXLOGLEN,"%s %s \n",inBuffer,timeBuffer);
    if (svrPref->IsScreenLoggingEnabled())
        printf("%s",tempBuffer);
    sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
    return 0;
}

int LogError(UInt32 inVerbosity,char* inBuffer)
{
    if(!inBuffer) return -1;
    UInt16 verbLvl = (UInt16) inVerbosity;
    if (verbLvl >= IllegalVerbosity)
    	verbLvl = FatalVerbosity;
    if (svrPref->GetErrorLogVerbosity() >= inVerbosity)
    {
        CheckErrorLogState();
        if (sErrorLog == NULL)
            return 0;
        char tempBuffer[MAXLOGLEN];
        char timeBuffer[MaxTimeBufferSize];
        if(!QTSSRollingLog::FormatDate(timeBuffer,false))
            return -1;
        snprintf(tempBuffer,MAXLOGLEN,"%s%s%s\n",timeBuffer,sErrorLevel[verbLvl],inBuffer);
        if (svrPref->IsScreenLoggingEnabled())
            printf("%s",tempBuffer);
        sErrorLog->WriteToLog(tempBuffer, kAllowLogToRoll);
    }
    return 0;
}

int LogError(UInt32 inVerbosity,const char* fmt,...)
{
    Assert(fmt != NULL);
    char inBuffer[MAXLOGLEN];
    va_list args;
     va_start(args,fmt);
		int length = ::vsnprintf(inBuffer, MAXLOGLEN, fmt, args);
		va_end(args);
		
		if (length < 0)
			return -1;
    return LogError(inVerbosity,inBuffer);
}

void CheckErrorLogState()
{
    //this function makes sure the logging state is in synch with the preferences.
    //extern variable declared in QTSSPreferences.h
    if ((NULL == sErrorLog) && (svrPref->IsErrorLogEnabled()))
    {
        sErrorLog = NEW ADDRErrorLog();
        sErrorLog->EnableLog();
    }
    if (NULL != sErrorLog&&(!svrPref->IsErrorLogEnabled()))
    {
        sErrorLog->Delete(); //sErrorLog is a task object, so don't delete it directly
        sErrorLog = NULL;
    }
}



