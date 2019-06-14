/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 2013-2014 gvtv Inc.  All Rights Reserved.
 *
 *
 * @gvtv@
 *
 */
/*
    File:       ADDRServerSocket.cpp

*/
#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/uio.h>
#endif


#include "DSTCPServerSocket.h"
#include "OSMemory.h"
#include "base64.h"
#include "MyAssert.h"

#define Server_SOCKET_DEBUG 0
UInt32   TCPServerSocket::sActiveConnections = 0;
TCPServerSocket::TCPServerSocket(UInt32 inSocketType)
:   TCPSocket(NULL,inSocketType),
    fEventMask(0),
    fSendBuffer(fSendBuf, 0),
    fSentLength(0)
{sActiveConnections++;}

OS_Error TCPServerSocket::Sendall(char* inData, const UInt32 inLength)
{
    iovec theVec[1];
    theVec[0].iov_base = (char*)inData;
    theVec[0].iov_len = inLength;
    
    return this->SendV(theVec, 1);
}

OS_Error TCPServerSocket::SendSendBuffer()
{
    OS_Error theErr = OS_NoErr;
    UInt32 theLengthSent = 0;
    
    if (fSendBuffer.Len == 0)
        return OS_NoErr;
    
    do
    {
        // theLengthSent should be reset to zero before passing its pointer to Send function
        // otherwise the old value will be used and it will go into an infinite loop sometimes
        theLengthSent = 0;
        //
        // Loop, trying to send the entire message.
        theErr = TCPSocket::Send(fSendBuffer.Ptr + fSentLength, fSendBuffer.Len - fSentLength, &theLengthSent);
        fSentLength += theLengthSent; 
    } while (theLengthSent > 0&&fSendBuffer.Len - fSentLength > 0);

    if (theErr == OS_NoErr)
        fSendBuffer.Len = fSentLength = 0; // Message was sent
    else
    {
        // Message wasn't entirely sent. Caller should wait for a read event on the POST socket
#ifdef _EPOLL
        fEventMask = EPOLLOUT|EPOLLET;
#else
        fEventMask = EV_WR;
#endif
    }
    
    return theErr;
}

void TCPServerSocket::SetOptions(int sndBufSize,int rcvBufSize)
{   //set options on the socket

    //qtss_printf("TCPServerSocket::SetOptions sndBufSize=%d,rcvBuf=%d,keepAlive=%d,noDelay=%d\n",sndBufSize,rcvBufSize,(int)keepAlive,(int)noDelay);
    int err = 0;
    err = ::setsockopt(TCPSocket::GetSocketFD(), SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(int));
    AssertV(err == 0, OSThread::GetErrno());

    err = ::setsockopt(TCPSocket::GetSocketFD(), SOL_SOCKET, SO_RCVBUF, (char*)&rcvBufSize, sizeof(int));
    AssertV(err == 0, OSThread::GetErrno());

#if __FreeBSD__ || __MacOSX__
    struct timeval time;
    //int len = sizeof(time);
    time.tv_sec = 0;
    time.tv_usec = 0;

    err = ::setsockopt(TCPSocket::GetSocketFD(), SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(time));
    AssertV(err == 0, OSThread::GetErrno());

   err = ::setsockopt(TCPSocket::GetSocketFD(), SOL_SOCKET, SO_SNDTIMEO, (char*)&time, sizeof(time));
    AssertV(err == 0, OSThread::GetErrno());
#endif

}

OS_Error TCPServerSocket::SendV(iovec* inVec, UInt32 inNumVecs)
{
    if (fSendBuffer.Len == 0)
    {
        
        for (UInt32 count = 0; count < inNumVecs; count++)
        {
            ::memcpy(fSendBuffer.Ptr + fSendBuffer.Len, inVec[count].iov_base, inVec[count].iov_len);
            fSendBuffer.Len += inVec[count].iov_len;
            Assert(fSendBuffer.Len < kSendBufferLen);
        }
    }
    return this->SendSendBuffer();
}
            
OS_Error TCPServerSocket::Reads(void* inBuffer, const UInt32 inLength, UInt32* outRcvLen)
{
    OS_Error theErr = TCPSocket::Read(inBuffer, inLength, outRcvLen);
    if (theErr != OS_NoErr)
#ifdef _EPOLL
        fEventMask = EPOLLIN|EPOLLET;
#else
        fEventMask = EV_RE;
#endif
        
    return theErr;
}

