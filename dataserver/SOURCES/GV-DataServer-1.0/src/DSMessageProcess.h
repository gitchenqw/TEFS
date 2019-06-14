
#ifndef __DATAMESSAGEPRO_H__
#define __DATAMESSAGEPRO_H__
#include "NsMessage.h"
#include <stdio.h>  
#include <OS.h>
#include "DSTCPServerSocket.h"

class MessageUtility
{
public:
    static int MessageTransaction(TCPServerSocket* fSocket,char* fRecvBuffer,UInt32 fRecvLen);
    static int notify_ns_del_ok(const DeleteCommand* cmd);
    static int notify_ns_backup_ok(int64_t fileid,int32_t chunkid);
public:
    static UInt32        BadPacketNum;
    
};
#endif