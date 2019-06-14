/*

 * Authors:
 *   yushuia <yushuai@gvtv.com>
 *      - initial release
 *      - 2015-02-10
 *
 */     
#ifndef _MESSAGE_H_
#define _MESSAGE_H_
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "serialization.h"
#include "defaultpref.h"


#define PROXYSERVICE 0
#define DATASERVICE  1

typedef struct ip_port
{
#if IPV6
    char    ip[sizeof(struct in6_addr)]; 
#else
    char    ip[4];
#endif
    int32_t port;
}address_t;

struct blockinfo
{
    int64_t             fileid;
    int32_t             chunkid;
    int32_t             blocksize;
    static   int32_t   MaxBlockSize;
};

enum
{
    //from client
    CLIENT_PUT = 0x0101,
    CLIENT_GET = 0x0102,
    //internal
    DS_BACKUP_CHUNK_PUSH = 0x0103,
    //nameserver
    DS_REGISTER = 0x0001,
    NS_COMMAND_CHUNK_DEL = 0x0005,
    NS_COMMAND_CHUNK_DEL_OK = 0x0006,
    NS_COMMAND_CHUNK_BACKUP = 0x0007,
    NS_COMMAND_CHUNK_BACKUP_OK = 0x0008,
    DS_HEARTBEAT = 0x0009,
    UNKNOWN
};
enum returncode
{
    PUTOK   = 200,
    GETOK   = 200,
    PUSHOK  = 200,
    PUTERROR = 400,
    GETERROR = 400,
    NOTFOUND = 404,
    INTERNALERROR = 500
};

enum 
{
    PacketLengthOffset = 0,
    PacketIdOffset = 4,
    PacketTypeOffset = 8,
    PacketEncryptOffset=12,
    PacketBodyOffset = 16
};
static inline int GenerateRequestId()
{
    return 0;
}
static inline int GetRequestLen(char* fReceiverPacketBuffer) 
{
    return ntohl(*(int*)&fReceiverPacketBuffer[PacketLengthOffset]);
}
static inline int GetRequestId(char* fReceiverPacketBuffer) 
{
    return ntohl(*(int*)&fReceiverPacketBuffer[PacketIdOffset]);
}
static inline int GetRequestType(char* fReceiverPacketBuffer) 
{
    return ntohl(*(int*)&fReceiverPacketBuffer[PacketTypeOffset]);
}
static inline int GetRequestEncrypt(char* fReceiverPacketBuffer) 
{
    return *(int*)&fReceiverPacketBuffer[PacketEncryptOffset];
}

static int CheckRecvMessgae(char* fRecvPacket,int fRecvPacketLen)
{
    int len = ntohl(*reinterpret_cast<int*>(fRecvPacket));
    if(fRecvPacketLen >= len)
    {
        return 0;
    }
    else 
    {
        return -1;
    }
}

struct message_base
{
    int32_t length;
    int32_t request_id;
    int32_t type;
    int32_t encrypt;
    message_base()
    {
        memset(this, 0, sizeof(message_base));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        printf("  lenth:%u\n",length);
        printf("  request_id:%d\n",request_id);
        printf("  messagetype:0x%x\n",type);
        printf("  encrypt:%s\n",(encrypt? "yes":"no"));
    }
};
#endif
