
#ifndef __ADDRSERVER_H__
#define __ADDRSERVER_H__

#include "DSServerPref.h"
#include "DSTCPListener.h"
#include "DSErrorLog.h"
#include "atomic.h"

#include "OSMutex.h"

class ADDRServer
{
public:
    enum ServerType
    {
        kUDPServerType,
        kTCPServerType
    };
    enum { NameServerMaxNum = 32 };
    ADDRServer(): 
    fTCPListeners(NULL),
    fTCPNumListeners(1),
    fUDPNumListeners(1),
    TCPaddr(0),
    TCPport(0),
    UDPaddr(0),
    UDPport(0),
    NameServerNum(0),
    fClientSessions(0),
    fLastBandwidthTime(0)
    {}
    static ADDRServer*  GetServer()
    { 
        if(!Server)
            Server = new ADDRServer();
        return Server;
    }
    void Initialize( ServerType type,UInt16 thePort,bool DontFork,UInt32 verbose);
    void setaddr( UInt32 address,UInt16 listen_port);
    void StartListeners();
    void StartServer();
    void IncrementClientSessions()
            { OSMutexLocker locker(&fMutex); fClientSessions++;} 
    
    void IncrementRcvBytes(UInt32 bytes)
                                         { (void)atomic_add(&fPeriodicRcvBytes, bytes); }
    void IncrementSndBytes(UInt32 bytes)
                                         { (void)atomic_add(&fPeriodicSndBytes, bytes); }
    UInt32 GetNumClientSessions()        { return fClientSessions; }  
    UInt64 GetPeriodicRcvBytes()         { return fPeriodicRcvBytes; }
    UInt64 GetPeriodicSndBytes()         { return fPeriodicRcvBytes; }
    
    UInt32 GetCurRcvBandwidthInBits()    { return fCurrentRcvBandwidthInBits; }
    UInt32 GetCurSndBandwidthInBits()    { return fCurrentSndBandwidthInBits; }
    int    CalcBindWidth();
public:
    static ADDRServer*   Server;
private:
    ADDR_TCPListenerSocket** fTCPListeners;
    UInt32 fTCPNumListeners;
    UInt32 fUDPNumListeners;
    UInt32 TCPaddr;
    UInt16 TCPport;
    UInt32 UDPaddr;
    UInt16 UDPport;
    ServerType           SessionType;
    UInt32               verboseLevel;
    bool                inDontFork;
    UInt32               NameServerIP[NameServerMaxNum];
    UInt16               NameServerNum;
private:
    OSMutex              fMutex;
    //stores the total number of client connections now.
    UInt32               fClientSessions;
    
    unsigned int       fPeriodicRcvBytes;
    unsigned int       fPeriodicSndBytes;
    //stores the current served bandwidth in BITS per second
    UInt32              fCurrentRcvBandwidthInBits;
    UInt32              fCurrentSndBandwidthInBits;
    SInt64              fLastBandwidthTime;
};
// write pid to file
void WritePid(Bool16 forked);

// clean the pid file
void CleanPid(Bool16 force);

#endif