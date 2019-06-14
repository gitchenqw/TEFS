#include "DSTCPSession.h"
#include <sys/types.h>
#include "OS.h" 
#include "Message.h"
#include "bserveripPref.h"
#include "DSMessageProcess.h"
#include <sys/syscall.h>

#define __debug__ 0
#define gettid() syscall(__NR_gettid) 

UInt32       DSTCPSession::TotalTaskID = 0;
UInt32       DSTCPSession::sActiveConnections = 0;

SInt64 DSTCPSession::Run()
{
#if __debug__
    printf("client: %d come in\n",TaskID);
    //printf("task is on thread %d\n",gettid());
#endif
    EventFlags events = this->GetEvents();
    if (events & Task::kKillEvent)
        return -1;
     if (events & Task::kTimeoutEvent)
        return -1;
    if(fState == kInitial)
        fState = kRequestReceiving;
    OS_Error theErr = this->ProcessEvent(Task::kReadEvent);
    if((theErr == EINPROGRESS) || (theErr == EAGAIN))
    { 
        fSocket->RequestEvent();
        return 0;
    }
#if __debug__
       printf("client: %d exit\n",TaskID);
#endif
        return -1;
}

OS_Error DSTCPSession::ProcessEvent(int /*eventBits*/)
{
    OS_Error theErr = OS_NoErr;
    if(!fSocket->IsConnected())
        return -1; 
    do
    {
        if(fState == kRequestReceiving)
        {
            UInt32 theRecvLen = 0;
            theErr = fSocket->Reads(&fRecvBuffer[fRecvLen], kReqBufSize - fRecvLen, &theRecvLen); 
            fRecvLen += theRecvLen;
            if (theErr == EAGAIN)
            {
                return theErr;
            }
            if(theErr != OS_NoErr)
            {
                return theErr;
            }
            if(fRecvLen >= 4)
            {
                fRequestLen = GetRequestLen(fRecvBuffer);    
                if((int)fRecvLen >= fRequestLen)
                {
                    fState = kRequestProcessing;
                } 
            }
        }    
        if(fState == kRequestProcessing)
        {
            MessageType = GetRequestType(fRecvBuffer);
            return MessageUtility::MessageTransaction(fSocket,fRecvBuffer,fRecvLen);
        }
    }while(1);
}



