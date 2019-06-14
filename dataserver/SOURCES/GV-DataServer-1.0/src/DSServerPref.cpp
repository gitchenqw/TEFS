#include "OS.h"
#include "DSServerPref.h"

void  ServerPref::ParseAttributes()
{
    Bool16 configFileError = InitFromConfigFile(configFilePath);
    if(configFileError)
    {
        printf("Couldn't find loadserver config file at: %s\n", configFilePath);
        ::exit(-1);
    }
    else
    {
        for (UInt32 x = 0; true; x++)
        {
            char* theValue = GetValueAtIndex(x);
            char* theKey = GetKeyAtIndex(x);
                if (theKey == NULL)
                    break;
            
            if (theValue != NULL)
            {
                if (::strcmp("dataserverid", theKey) == 0 )
                {
                    ::sscanf(theValue, "%" _S32BITARG_ "", &dataserverid);
                }
                else if (::strcmp("localip", theKey) == 0 )
                {
                    localip = theValue;
                }
                else if (::strcmp("localport", theKey) == 0 )
                {
                    UInt32 tempPort = 0;
                    ::sscanf(theValue, "%" _U32BITARG_ "", &tempPort);
                    localport = (UInt16)tempPort;
                }
                else if (::strcmp("shouldrestart", theKey) == 0)
                {
                    if (::strcmp("yes", theValue) == 0)
                        RestartEnabled = true;
                }
                else if (::strcmp("nameserverdomain", theKey) == 0)
                {
                    NameServerDomain = theValue;
                }
                else if (::strcmp("nameserverport", theKey) == 0)
                {
                     UInt32 tempPort = 0;
                    ::sscanf(theValue, "%" _U32BITARG_ "", &tempPort);
                    NameServerPort = (UInt16)tempPort;
                }
                else if (::strcmp("shouldlog", theKey) == 0)
                {
                    if (::strcmp("yes", theValue) == 0)
                        fErrorLogEnabled = true;
                }
                else if (::strcmp("ScreenLoggingEnabled", theKey) == 0)
                {
                    if (::strcmp("yes", theValue) == 0)
                        fScreenLoggingEnabled = true;
                }
                else if (::strcmp("logdir", theKey) == 0)
                {
                    logdir = theValue;
                }
                else if (::strcmp("pidfilepath", theKey) == 0)
                {
                    pidfilepath = theValue;
                }
                else if (::strcmp("logname", theKey) == 0)
                {
                    logname = theValue;
                }
                else if (::strcmp("rollinday", theKey) == 0)
                {
                    UInt32 tempPort = 0;
                    ::sscanf(theValue, "%" _U32BITARG_ "", &tempPort);
                    rollinday = (UInt16)tempPort;
                }
                else if (::strcmp("maxlogbytes", theKey) == 0)
                {
                    ::sscanf(theValue, "%" _U32BITARG_ "", &maxlogbytes);
                }
                else if (::strcmp("Verbosity", theKey) == 0)
                {
                    ::sscanf(theValue, "%" _U32BITARG_ "", &fPrefVerbosity);
                }
                else if (::strcmp("DataBaseUserName", theKey) == 0)
                {
                    DBUserName = theValue;
                }
                else if (::strcmp("DataBasePassWord", theKey) == 0)
                {
                    DBPassWord = theValue;
                }
                else if (::strcmp("DataBaseServerIP", theKey) == 0)
                {
                    DBServerIP = theValue;
                }
                    
                else if (::strcmp("DataBaseName", theKey) == 0)
                {
                    DBName = theValue;
                }
                else if(::strcmp("DataBaseRootPassWord", theKey) == 0)
                {
                    DBRootPassWord = theValue;
                }
                else if(::strcmp("MountRootPath", theKey) == 0)
                {
                    MountRootPath = theValue;
                }
                else
                    printf("Found bad directive in LoadServer config file: %s\n", theKey);
            }
        }
    }
}


