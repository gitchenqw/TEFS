
#ifndef __NAMESERVERSSION_H__
#define __NAMESERVERSSION_H__

#include "DSTCPServerSocket.h"
#include "IdleTask.h"
#include "NsMessage.h"

class NameServerSession : public IdleTask
{
public:
    
    enum 
    { 
        kInitial,
        kSendRegister,
        kRecvRegisterResponse,
        kConnected,
    };
    enum
    {
        kReqBufSize = 1024,
        kMaxPacketSize = 1024
    };
    NameServerSession(SInt64 ms,UInt32 ip,UInt16 port):     
    IdleTask(),
    fState(kInitial),
    fRecvLen(0),
    fRequestLen(0),
    fPacketSendLen(0),
    HeartBeatTime(ms),
    IdleTime(5000),
    MulIdleTime(0),
    NameServerIP(ip),
    NameServerPort(port),
    sendbusy(false)
    {
        this->SetTaskName("NameServerSession");
        ::memset(fRecvBuffer, 0,kReqBufSize + 1);
        in_addr in;
        in.s_addr = NameServerIP;
        ::memcpy(NameServerIPStr,inet_ntoa(in),strlen(inet_ntoa(in)));
        this->Signal(kStartEvent);
    }
    
    ~NameServerSession() 
    {
        if(fSocket)
            delete fSocket;
        ClearMessageFromQueue();
    }
    virtual SInt64                      Run();
    static   void                      GetNameServerIp(UInt32 iplist[],UInt16 * numofip);
    int                                 packet_dataserver_resgister();
    int                                 packet_dataserver_heatbeat();
    void                               AddMessageToQueue(char* fPacket,UInt32 fPacketLen);
    void                               GetMessageFromQueue(char*& fPacket,UInt32& fPacketLen);
    int32_t                             calc_load_weight();
private:
    void                                again(); 
    void                                ClearMessageFromQueue();
    
private:
    TCPServerSocket*    fSocket;
    UInt32              fState;
    DsRegister          RegisterPacket;   
    DsRegisterRes       RegisterResPacket;
    // request for receive  
    char                fRecvBuffer[kReqBufSize+1];
    UInt32              fRecvLen;
    UInt32              fRequestLen;
    //for packet send
    char                fPacketSend[kMaxPacketSize];
    UInt32               fPacketSendLen;
    SInt64               HeartBeatTime;
    SInt64               IdleTime;
    SInt64               MulIdleTime;
    UInt32                NameServerIP;
    char                 NameServerIPStr[20];
    UInt16               NameServerPort;
    //Message queue,eg:push_ok or delete_ok
    OSQueue_Blocking     fMessageQueue;
    char*                ResMessage;
    UInt32               ResMessageLen;
    bool                sendbusy;
};
#endif
