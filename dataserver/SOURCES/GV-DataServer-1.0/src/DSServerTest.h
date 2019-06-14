#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include "TCPSocket.h"
#include "UDPSocket.h"
#include "ClientMessage.h"
#include "NsMessage.h"
#include "DSHeader.h"

#define TCPRCVWINSIZE          128*1024
#define recvbufferlen           TCPRCVWINSIZE*5/4
#define MAXsendbuf          1024
#define DEFAULTPORT         5800

extern int debuglevel;
class ADDRServerTest
{
    enum
    {
        TCP,
        UDP,
        ALL
    };
public:
    ADDRServerTest(int m_type,UInt32 ip = INADDR_ANY,UInt16 port=DEFAULTPORT,char* infile=NULL):
    messagetype( m_type),
    remote_ip(ip),
    remote_port(port),
    sendbuflen(0),
    sendbytes(0),
    receivebytes(0),
    recvheadlen(0),
    Method(ALL),
    inputfile(infile),
    c_put(0),
    c_get(0),
    successflag(false)
    {
        memset(sendbuf,'\0',sizeof(sendbuf));
        memset(recvbuf,'\0',sizeof(recvbuf));
    }
    ~ADDRServerTest()
    {
        if(c_put) free(c_put);
    }
    int TCPSession();
    int UDPSession();
    int TCPRead(TCPSocket& fSocket);
    int TCPReadDataToFile(Socket* fSocket,int filefd,int datalen);
    void   PacketMessage();
    int    ParseMessage(Socket* fSocket);
    UInt32 get_receive_bytes(){return receivebytes;}
    UInt32 get_send_bytes(){return sendbytes;}
    
private:
    int                     messagetype;
    UInt32                  remote_ip;
    UInt16                  remote_port;
    char                   sendbuf[MAXsendbuf];
    UInt32                 sendbuflen;
    char                   recvbuf[recvbufferlen];

    UInt32                 sendbytes;
    UInt32                 receivebytes; 
    UInt32                 recvheadlen;
    int                    Method;
    int                    fstate; 

private:
    char*                  inputfile;
    ClientPut*              c_put;
    ClientGet*              c_get;
    DeleteCommand           ns_del;
    BackupCommand           ns_backup;
    
    SInt64                  LastReadTime;

public:
    bool                  successflag;
    static UInt32         BadPacket;
    static UInt32         UnKnownPacket;
    static int            RequestID;
    static int            ReadError;
    static int            ReadTimeOutError;

};

int             ADDRServerTest::RequestID = 0;
UInt32          ADDRServerTest::BadPacket = 0;
UInt32          ADDRServerTest::UnKnownPacket = 0;
int             ADDRServerTest::ReadError = 0;
int             ADDRServerTest::ReadTimeOutError = 0;
    

