
#include <stdio.h>
#include <stdlib.h> 
#include <sys/wait.h>
#include "DSServerPref.h"
#include "DSServer.h"
#include "defaultpref.h"
#include "bserveripPref.h"
#include "data_management.h"
#include "NameServersession.h"

ServerPref* svrPref = NULL;
NameServerSession*   NSsession;
tfs::dataserver::DataManagement* datamanager = NULL;

static int      sSigIntCount = 0;
static int      sSigTermCount = 0;
static pid_t    sChildPID = 0;

static char*   sDefaultConfigFilePath = DEFAULTPATHS_ETC_DIR "dataserver.conf";
static ADDRServer::ServerType theServerType = ADDRServer::kTCPServerType;
static char* theConfigFilePath;
static Bool16 theServerTypeIsSpecified = false;
static Bool16 thePortIsSpecified = false;

static void usage();
static Bool16 RestartServer(char* theXMLFilePath);

void usage()
{
    printf("-d: Run in the foreground\n");
    printf("-t: the transport type: udp,tcp\n");
    printf("-p: XXX: Specify the default UDP listening port of the server\n");
    printf("-c: Specify a config,fileDefaults to loadserver.conf\n"); 
    printf("-V: debug verbose level\n");
    printf("-v: Print version\n");
    printf("-h: Prints usage\n");
}

Bool16 sendtochild(int sig, pid_t myPID);
Bool16 sendtochild(int sig, pid_t myPID)
{
    if (sChildPID != 0 && sChildPID != myPID) // this is the parent
    {   // Send signal to child
        ::kill(sChildPID, sig);
        return true;
    }

    return false;
}

void sigcatcher(int sig, int /*sinfo*/, struct sigcontext* /*sctxt*/);
void sigcatcher(int sig, int /*sinfo*/, struct sigcontext* /*sctxt*/)
{
#if DEBUG
    qtss_printf("Signal %d caught\n", sig);
#endif
    pid_t myPID = getpid();
    //
    // SIGHUP means we should reread our preferences
    if (sig == SIGHUP)
    {
        if (sendtochild(sig,myPID))
        {
            return;
        }
        else
        {
            // This is the child process.
            // Re-read our preferences.
            exit(1);
        }
    }
        
    //Try to shut down gracefully the first time, shutdown forcefully the next time
    if (sig == SIGINT) // kill the child only
    {
        if (sendtochild(sig,myPID))
        {
            return;// ok we're done 
        }
        else
        {
			// Tell the server that there has been a SigInt, the main thread will start
			// the shutdown process because of this. The parent and child processes will quit.
            sSigIntCount++;
            printf("dataserver exit with sigint\n");
            exit(0);
		}
    }
	
	if (sig == SIGTERM || sig == SIGQUIT) // kill child then quit
    {
        if (sendtochild(sig,myPID))
        {
             return;// ok we're done 
        }
        else
        {
			// Tell the server that there has been a SigTerm, the main thread will start
			// the shutdown process because of this only the child will quit
            
			sSigTermCount++;
		}
    }
}

Bool16 RestartServer(char* theXMLFilePath)
{
	 Bool16 autoRestart = true;
    thePortIsSpecified = false;
    if(svrPref) 
    {
        delete svrPref;
        svrPref = NULL;
    }
    svrPref = new ServerPref(theConfigFilePath);
    autoRestart = svrPref->IsRestartEnabled();
    return autoRestart;
}

int main(int argc, char *argv[])
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = (void(*)(int))&sigcatcher;
    (void)::sigaction(SIGPIPE, &act, NULL);
    (void)::sigaction(SIGHUP, &act, NULL);
    (void)::sigaction(SIGINT, &act, NULL);
    (void)::sigaction(SIGTERM, &act, NULL);
    (void)::sigaction(SIGQUIT, &act, NULL);
    (void)::sigaction(SIGALRM, &act, NULL);
    
    struct rlimit rl;
    // set it to the absolute maximum that the operating system allows - have to be superuser to do this
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit (RLIMIT_NOFILE, &rl);

    bool inDontFork = false;
    Bool16 configFilePathIsSpecified = false;
    UInt16 thePort = 0; //port can be set on the command line
    UInt32 verboseLevel = 0;
    char ch = 0;
    while ((ch = getopt(argc,argv, "dt:p:c:V:vh")) != EOF) // opt: means requires option arg
    {
        switch(ch)
        {
            case 'v':
                printf("%s %s, built on %s, %s.\n", Software,Version, __DATE__  , __TIME__);
                ::exit(0); 
            case 'd':
                inDontFork = true;
                break;                
           case 'V':
				verboseLevel = atoi(optarg);
				break;
            case 't':
                theServerTypeIsSpecified = true;
                if (::strcmp("tcp", optarg) == 0)
                    theServerType = ADDRServer::kTCPServerType;
                else if (::strcmp("udp", optarg) == 0)
                    theServerType = ADDRServer::kUDPServerType;
                else
                {
                    printf("Invalid transport type: %s\n", optarg);
                    exit(0);
                }
                break;
            case 'p':
                thePortIsSpecified = true;
                thePort = atoi(optarg);
                break;
            case 'c':
                configFilePathIsSpecified = true;
                theConfigFilePath = optarg;
                break;
            case 'h':
                usage();
                ::exit(0);
            default:
                break;
        }
    }
    if(!configFilePathIsSpecified )
        theConfigFilePath = sDefaultConfigFilePath;
    // Check port
    if (thePort < 0 || thePort > 65535)
    { 
        qtss_printf("Invalid port value = %d max value = 65535\n",thePort);
        exit (-1);
    }
    svrPref = new ServerPref(theConfigFilePath);
    if (!inDontFork)
    {
        if (daemon(0,0) != 0)
        {
            exit(-1);
        }
    }
    int status = 0;
    int pid = 0;
    pid_t processID = 0;
    
    if(!inDontFork)
    {
        do{
            processID = fork();
            Assert(processID >= 0);
            if (processID > 0) // this is the parent and we have a child
            {
                sChildPID = processID;
                status = 0;
                while (status == 0) //loop on wait until status is != 0;
                {	
                    pid =::wait(&status);
                    SInt8 exitStatus = (SInt8) WEXITSTATUS(status);
                    if (WIFEXITED(status) && pid > 0 && status != 0) // child exited with status -2 restart or -1 don't restart 
                    {
                        LogError(DebugVerbosity,"child exited with status=%d", exitStatus);
                        if ( exitStatus == -1) // child couldn't run don't try again
                        {
                            //qtss_printf("child exited with -1 fatal error so parent is exiting too.\n");
                            exit (EXIT_FAILURE); 
                        }
                        break; // restart the child
                    }
                
                    if (WIFSIGNALED(status)) // child exited on an unhandled signal (maybe a bus error or seg fault)
                    {	
                        LogError(DebugVerbosity,"child exited on an unhandled signal(%d)",WIFSIGNALED(status));
                        break; // restart the child
                    }

                 		
                    if (pid == -1 && status == 0) // parent woken up by a handled signal
                    {
                        //qtss_printf("handled signal continue waiting\n");
                        continue;
                    }
                   	
                    if (pid > 0 && status == 0)
                    {
                        //qtss_printf("child exited cleanly so parent is exiting\n");
                        exit(EXIT_SUCCESS);                		
                    }
                    LogError(DebugVerbosity,"child died for unknown reasons parent is exiting");
                    exit (EXIT_FAILURE);
                }
            }
            else if (processID == 0) // must be the child
            {
                break;
            }
            else
                exit(EXIT_FAILURE);
            	
            //eek. If you auto-restart too fast, you might start the new one before the OS has
            //cleaned up from the old one, resulting in startup errors when you create the new
            //one. Waiting for a second seems to work
            sleep(1);
        } while (RestartServer("theXMLFilePath")); // fork again based on pref if server dies
        if (processID != 0) //the parent is quitting
            exit(EXIT_SUCCESS);
    }
    sChildPID = 0;
    //we have to do this again for the child process, because sigaction states
    //do not span multiple processes.
    (void)::sigaction(SIGPIPE, &act, NULL);
    (void)::sigaction(SIGHUP, &act, NULL);
    (void)::sigaction(SIGINT, &act, NULL);
    (void)::sigaction(SIGTERM, &act, NULL);
    (void)::sigaction(SIGQUIT, &act, NULL);
    
    ADDRServer* server = ADDRServer::GetServer();
    if(!thePortIsSpecified)
        thePort = svrPref->GetLocalPort();
    if(!thePort)
        thePort = DEFAULTLocalPort;
    server->Initialize(theServerType,thePort,inDontFork,verboseLevel); 
    server->StartServer();
    CleanPid(false);
}
