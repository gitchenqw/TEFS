#include <netdb.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "TCPSocket.h"
#include "ClientMessage.h"
#include "DsMessage.h"
#include "DSMessageProcess.h"
#include "DSClientPutTask.h"
#include "DSClientGetTask.h"
#include "DSPutTask.h"
#include "DSPushTask.h"
#include "defaultpref.h"
#include "DSErrorLog.h"
#include "data_management.h"
#include "NameServersession.h"
#define __debug__ 1

UInt32  MessageUtility::BadPacketNum = 0;
extern tfs::dataserver::DataManagement* datamanager;
extern ServerPref*          svrPref;
extern NameServerSession*   NSsession;

int MessageUtility::MessageTransaction(TCPServerSocket* fSocket,char* fRecvBuffer,UInt32 fRecvLen)
{
    int MessageType = GetRequestType(fRecvBuffer);
    switch(MessageType)
    {
        case CLIENT_PUT: 
        {
            LogError(DebugVerbosity,"Receive Client PUT request from:%s",fSocket->GetRemoteAddrStr()->Ptr);
            int64_t pos = 0;
            ClientPut M_PutTemp;
            M_PutTemp.deserializehead(fRecvBuffer,fRecvLen,pos);
            pos = 0;
            ClientPut* M_Put = (ClientPut*) malloc(sizeof(struct ClientPut) +  M_PutTemp.backup_dataservernum*sizeof(address_t));
            M_Put->deserialize(fRecvBuffer,fRecvLen,pos); 
#if __debug__ 
            M_Put->dump();
#endif
            int32_t currentdatalen;
            currentdatalen = fRecvLen - pos;
            char* data_buffer = &fRecvBuffer[pos];                         
            new DSClientPutTask(M_Put,fSocket,data_buffer,currentdatalen);
            break;
        }
        case CLIENT_GET:
        {
            LogError(DebugVerbosity,"Receive Client GET request from:%s",fSocket->GetRemoteAddrStr()->Ptr);
            ClientGet* M_Get = (ClientGet*) malloc(sizeof(struct ClientGet));
            int64_t pos = 0;
            M_Get->deserialize(fRecvBuffer,fRecvLen,pos);
#if __debug__ 
            M_Get->dump();
#endif
            new DSClientGetTask(M_Get,fSocket);
            break;
        }
        case DS_BACKUP_CHUNK_PUSH:
        {
            LogError(DebugVerbosity,"Receive PUSH request from DataServer:%s",fSocket->GetRemoteAddrStr()->Ptr);
            DsPush M_Push;
            int64_t pos = 0;
            M_Push.deserialize(fRecvBuffer,fRecvLen,pos);  
#if __debug__ 
            M_Push.dump();
#endif
            int32_t currentdatalen;
            currentdatalen = fRecvLen - pos;
            char* data_buffer = &fRecvBuffer[pos];                         
            new DSPutTask(M_Push,fSocket,data_buffer,currentdatalen);
            break;
        }
        case DS_REGISTER:
        {
            LogError(DebugVerbosity,"Receive Register response from NameServer:%s",fSocket->GetRemoteAddrStr()->Ptr);
            DsRegisterRes res;
            int64_t pos = 0;
            res.deserialize(fRecvBuffer,fRecvLen,pos);
            int32_t tmp = res.retcode;                      //swap(retcode,encrypt) 
            res.retcode = res.encrypt;
            res.encrypt = tmp;
#if __debug__ 
            res.dump();
#endif
            if(res.blocksize < 0)
            {
                LogError(WarningVerbosity,"Invalid blocksize from NameServer,use default size:%d",DEFAULTBLOCKSIZE);
            }
            else
            {
                blockinfo::MaxBlockSize = res.blocksize;
            }
            break;
        }
        case NS_COMMAND_CHUNK_BACKUP:
        {
            BackupCommand cmd;
            int64_t pos = 0;
            cmd.deserialize(fRecvBuffer,fRecvLen,pos); 
            LogError(DebugVerbosity,"Receive Backup command from NameServer:%s",fSocket->GetRemoteAddrStr()->Ptr);
#if __debug__ 
            cmd.dump();
#endif
            SInt32 start = 0;                   // need to modify
            SInt32 end = 0;
            int32_t visit_count;
            int32_t blocksize;
            if(datamanager->get_block_info(cmd.fileid,cmd.chunkid,blocksize,visit_count) < 0)
                return -1;
            end = blocksize-1;

            new DSPushTask(cmd.dst_address.ip,cmd.dst_address.port,cmd.fileid,cmd.chunkid,start,end);
            break;
        }
        case NS_COMMAND_CHUNK_DEL:
        {
            DeleteCommand cmd;
            int64_t pos = 0;
            cmd.deserialize(fRecvBuffer,fRecvLen,pos); 
            LogError(DebugVerbosity,"Receive Delete command from NameServer:%s",fSocket->GetRemoteAddrStr()->Ptr);
#if __debug__ 
            cmd.dump();
#endif
            datamanager->del_single_block(cmd.fileid,cmd.chunkid);
            LogError(DebugVerbosity,"Delete block<%lld,%d> ok,notify nameserver",cmd.fileid,cmd.chunkid);
            notify_ns_del_ok(&cmd);
            break;
        }   
        default:
        {
            LogError(DebugVerbosity,"Unknown request from:%s",fSocket->GetRemoteAddrStr()->Ptr);
            ++MessageUtility::BadPacketNum;
            return -1;
        }
    }
    return 0;
}

int MessageUtility::notify_ns_del_ok(const DeleteCommand* cmd)
{
    char* fPacket=new char[DeleteCommandResLen];
    DeleteCommandRes Res;
    Res.length = DeleteCommandResLen;
    Res.request_id = cmd->request_id;
    Res.type = NS_COMMAND_CHUNK_DEL_OK;
    Res.fileid = cmd->fileid;
    Res.chunkid = cmd->chunkid;
    int32_t blockcount = 0;
    int64_t diskused = 0; 
    int64_t diskcapacity =0;
    datamanager->get_ds_filesystem_info(blockcount,diskused,diskcapacity);
    Res.diskfree = (diskcapacity - diskused)>>10;
    UInt32 fPacketLen = Res.length;
    int64_t pos = 0;
    if(Res.serialize(fPacket, fPacketLen, pos) < 0)
    {
        delete fPacket;
        return -1;
    }
    NSsession->AddMessageToQueue(fPacket,fPacketLen);
    NSsession->Signal(Task::kWriteEvent);
    return 0;
}

int MessageUtility::notify_ns_backup_ok(int64_t fileid,int32_t chunkid)
{
    int64_t pos = 0;
    BackupCommandRes res;
    res.length = BackupCommandResLen;
    res.type = NS_COMMAND_CHUNK_BACKUP_OK;
    res.request_id = GenerateRequestId();
    res.dataserverid = svrPref->GetDataserverId();
    res.fileid = fileid;
    res.chunkid = chunkid;
    int32_t blockcount = 0;
    int64_t diskused = 0; 
    int64_t diskcapacity =0;
    datamanager->get_ds_filesystem_info(blockcount,diskused,diskcapacity);
    res.diskfree = (diskcapacity - diskused)>>20;
    res.dump();
    char* fPacket=new char[BackupCommandResLen];
    UInt32 fPacketLen = res.length;
    if(res.serialize(fPacket, fPacketLen, pos) < 0)
    {
        delete fPacket;
        return -1;
    }
    NSsession->AddMessageToQueue(fPacket,fPacketLen);
    NSsession->Signal(Task::kWriteEvent);
    return 0;
}

static int GetIP_v4_and_v6_linux(int family, char *address, unsigned int size)
{
    struct ifaddrs *ifap0, *ifap;
    char buf[NI_MAXHOST];
    char *interface = "lo";
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    
    if( NULL == address )
    {
        return -1;
    }
    if(getifaddrs(&ifap0)) 
    {
        return -1;
    }
    for( ifap = ifap0; ifap != NULL; ifap=ifap->ifa_next)
    {
        if(strcmp(interface, ifap->ifa_name) == 0) continue;
        if(ifap->ifa_addr==NULL) continue;
        if ((ifap->ifa_flags & IFF_UP) == 0) continue;
        if(family != ifap->ifa_addr->sa_family) continue;

        if(AF_INET == ifap->ifa_addr->sa_family) 
        {
            addr4 = (struct sockaddr_in *)ifap->ifa_addr;
            if ( NULL != inet_ntop(ifap->ifa_addr->sa_family,(void *)&(addr4->sin_addr), buf, NI_MAXHOST) )
            {
                if(size <= strlen(buf) ) break;
                strcpy(address, buf);
                freeifaddrs(ifap0);
                return 0;
            }
            else break;
        }
        else if(AF_INET6 == ifap->ifa_addr->sa_family) 
        {
            addr6 = (struct sockaddr_in6 *)ifap->ifa_addr;
            if(IN6_IS_ADDR_MULTICAST(&addr6->sin6_addr))
            {
                continue;
            }
            if(IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr))
            {
                continue;
            }
            if(IN6_IS_ADDR_LOOPBACK(&addr6->sin6_addr))
            {
                continue;
            }
            if(IN6_IS_ADDR_UNSPECIFIED(&addr6->sin6_addr))
            {
                continue;
            }
            if(IN6_IS_ADDR_SITELOCAL(&addr6->sin6_addr))
            {
                continue;
            }
            if ( NULL != inet_ntop(ifap->ifa_addr->sa_family,(void *)&(addr6->sin6_addr), buf, NI_MAXHOST) )
            {
                if(size <= strlen(buf) ) break;
                strcpy(address, buf);
                freeifaddrs(ifap0);
                return 0;
            }
            else break;
        }
    }
    freeifaddrs(ifap0);
    return -1;
}
