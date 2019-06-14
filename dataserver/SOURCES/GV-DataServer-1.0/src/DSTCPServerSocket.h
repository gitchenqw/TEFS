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
    File:       ADDRServerSocket.h

*/


#ifndef __SERVER_SOCKET__
#define __SERVER_SOCKET__

#include "OSHeaders.h"
#include "TCPSocket.h"

class TCPServerSocket : public TCPSocket
{
    public:
        
        TCPServerSocket(UInt32 inSocketType);
        virtual ~TCPServerSocket(){}
        
        //
        // Implements the ClientSocket Send and Receive interface for a TCP connection
        virtual OS_Error    Sendall(char* inData, const UInt32 inLength);
        virtual OS_Error    Reads(void* inBuffer, const UInt32 inLength, UInt32* outRcvLen);
        
        virtual void    SetRcvSockBufSize(UInt32 inSize) { TCPSocket::SetSocketRcvBufSize(inSize); }
        virtual void    SetOptions(int sndBufSize = 8192,int rcvBufSize=1024);
        
        UInt32      GetEventMask()          { return fEventMask; }
    protected:
    
        // Generic open function
        OS_Error    SendV(iovec* inVec, UInt32 inNumVecs);
        OS_Error    SendSendBuffer();
        
        UInt32      fEventMask;

        enum
        {
            kSendBufferLen = 1024*16
        };
        
        // Buffer for sends.
        char        fSendBuf[kSendBufferLen + 1];
        StrPtrLen   fSendBuffer;
        UInt32      fSentLength;
private:
        static UInt32    sActiveConnections;
        
};

#endif //__SERVER_SOCKET__
