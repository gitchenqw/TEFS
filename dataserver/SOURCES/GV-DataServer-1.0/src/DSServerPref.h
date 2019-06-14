#ifndef __DATASERVERPREF_H__
#define __DATASERVERPREF_H__
#include "defaultpref.h"
#include "FilePrefsSource.h"
#include "DSHeader.h"
#include "defaultpref.h"

class ServerPref:public FilePrefsSource
{
public:
    ServerPref(char*configFile,Bool16 allowDuplicates=false):
    FilePrefsSource(allowDuplicates),
    configFilePath(configFile),
    dataserverid(DATASERVERID),
    localip(NULL),
    localport(DEFAULTLocalPort),
    NameServerDomain(DEFAULTNameServerDomain),
    NameServerPort(DEFAULTNameServerPort),
    logname(NULL),
    logdir(NULL),
    pidfilepath(NULL),
    rollinday(ROLLINDAY),
    maxlogbytes(MAXLOGBYTES),
    fPrefVerbosity(MessageVerbosity),
    fScreenLoggingEnabled(false),
    fErrorLogEnabled(false),
    RestartEnabled(false),
    DBUserName(NULL),
    DBPassWord(NULL),
    DBServerIP(NULL),
    DBName(NULL),
    DBRootPassWord(NULL),
    MountRootPath(MOUNTROOTPATH)
    {
        ParseAttributes();
    }
    void        ParseAttributes();
    SInt32      GetDataserverId()           {return dataserverid;}
    char*       GetLocalIp()               {return localip;}
    UInt16       GetLocalPort()             {return localport;}
    char*       GetNameServerDomain()       {return NameServerDomain;}
    UInt16      GetNameServerPort()         {return NameServerPort;}
    char*      GetErrorLogName()           {return logname; }
    char*      GetErrorLogDir()            {return logdir;}
    char*      GetPidFilePath()            {return pidfilepath;}
    UInt32     GetRollIntervalInDays()     {return rollinday;}
    UInt32     GetMaxErrorLogBytes()       {return maxlogbytes;}
    UInt32     GetErrorLogVerbosity()      {return fPrefVerbosity;}
    bool      IsScreenLoggingEnabled()    {return fScreenLoggingEnabled;}
    bool      IsErrorLogEnabled()         {return fErrorLogEnabled; }
    bool      IsRestartEnabled()          {return RestartEnabled; }
    char*     GetDBUserName()             {return DBUserName;}
    char*     GetDBPassWord()             {return DBPassWord;}
    char*     GetDBServerIP()             {return DBServerIP;}
    char*     GetDBName()                 {return DBName;}
    char*     GetDBRootPassWord()         {return DBRootPassWord;}
    char*     GetMountRootPath()          {return MountRootPath;}
       
private:
    char*                     configFilePath; 
//address
    SInt32                     dataserverid;
    char*                     localip;
    UInt16                    localport;
    char*                     NameServerDomain;
    UInt16                    NameServerPort;
//log
    char*                    logname;
    char*                    logdir;
    char*                    pidfilepath;
    UInt16                    rollinday;
    UInt32                    maxlogbytes;
    UInt32                    fPrefVerbosity;
    bool                     fScreenLoggingEnabled;
    bool                     fErrorLogEnabled;
    bool                     RestartEnabled;
//DB
    char*                    DBUserName;
    char*                    DBPassWord;
    char*                    DBServerIP;
    char*                    DBName;
    char*                    DBRootPassWord;
//disk manage
    char*                   MountRootPath;
};

#endif