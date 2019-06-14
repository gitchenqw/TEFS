/*
 *
 * @
 *
 * Copyright (c) 2013-2014 gvtv Inc.  All Rights Reserved.
 *
 *
 */
/*
    File:       TCPListener.cpp

    Contains:   Implements object defined in 
    

*/
#include "DSTCPListener.h"
#include "DSClientPutTask.h"

Task*   ADDR_TCPListenerSocket::GetSessionTask(TCPSocket** outSocket)
{
    Assert(outSocket != NULL);
    DSTCPSession* theTask = NEW DSTCPSession();
    *outSocket = theTask->GetSocket();  
    if (this->OverMaxConnections())
        this->SlowDown();
    else
        this->RunNormal();  
    return theTask;
    
}

Bool16 ADDR_TCPListenerSocket::OverMaxConnections()
{
    return false; 
}
