/*
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *      - initial release
 * Warning: delete backup response are not 8 bytes aligned
 *
 */
#ifndef GV_NS_MESSAGE_H_
#define GV_NS_MESSAGE_H_
#include "Message.h"
#include "OS.h"

#define BackupCommandResLen 44
#define DeleteCommandResLen 40

struct DsRegister:message_base
{
    int32_t servicetype;
    int32_t dataserverid;
    int64_t diskfree;     //Unit:MB
    int64_t diskcapacity; 
    int32_t port;
    int32_t endflag;
    DsRegister()
    {
        memset(this, 0, sizeof(DsRegister));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /REGISTER,dataserverid:%d,diskcapaciy:%" PRId64 ",diskfree:%" PRId64 ",port:%d\n",
                dataserverid,diskcapacity,diskfree,port);
    }
};

struct DsRegisterRes:message_base  // infact swap(retcode,encrypt) 
{
    int32_t retcode;
    int32_t blocksize;
    int32_t endflag;
    DsRegisterRes()
    {
        memset(this, 0, sizeof(DsRegister));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /REGISTERRES,return code:%d\n",retcode);
        printf("  blocksize:%d\n",blocksize);
    }
};

struct DeleteCommand:message_base
{
    int64_t fileid;
    int32_t chunkid;
    int32_t endflag; 
    DeleteCommand()
    {
        memset(this, 0, sizeof(DeleteCommand));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /DELETE,blockid:<%" PRId64 ",%d>\n",fileid,chunkid);
    }
};

struct DeleteCommandRes:message_base//not 8 bytes aligned
{
    int64_t fileid;
    int32_t chunkid;
    int64_t diskfree;
    int32_t endflag;
    DeleteCommandRes()
    {
        memset(this, 0, sizeof(DeleteCommandRes));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /DELETERES,blockid:<%" PRId64 ",%d>,diskfree:%" PRId64 "\n",fileid,chunkid,diskfree);
    }
};

struct BackupCommand:message_base
{
    int64_t fileid;
    int32_t chunkid;
    address_t src_address;
    address_t dst_address;
    int32_t endflag;
    BackupCommand()
    {
        memset(this, 0, sizeof(BackupCommand));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        void* src_ip;
        void* dst_ip;
#if IPV6
        char  src_point[INET6_ADDRSTRLEN];
        char  dst_point[INET6_ADDRSTRLEN];
        int   domain = AF_INET6;
#else 
        char src_point[INET_ADDRSTRLEN];
        char dst_point[INET_ADDRSTRLEN];
        int  domain = AF_INET;
        int32_t src_ip4 = htonl(*(int32_t*)src_address.ip);
        int32_t dst_ip4 = htonl(*(int32_t*)dst_address.ip);
        src_ip = &src_ip4;
        dst_ip = &dst_ip4;
#endif
        if(OS::NetWorkToPoint(domain,src_ip,src_point) != NULL&&
                OS::NetWorkToPoint(domain,dst_ip,dst_point) != NULL) 
        {
            printf("  Method /BACKUPCOMMAND,blockid<%" PRId64 ",%d>,src:<%s,%d>,dst:<%s,%d>\n",
                fileid,chunkid,src_point,src_address.port,dst_point,dst_address.port);
        }
    }
};

struct BackupCommandRes:message_base//not 8 bytes aligned
{
    int32_t dataserverid;
    int64_t fileid;
    int32_t chunkid;
    int64_t diskfree;
    int32_t endflag;
    BackupCommandRes()
    {
        memset(this, 0, sizeof(BackupCommandRes));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /BACKUPCOMMANDRES,dataserverid:%d,blockid:<%" PRId64 ",%d>,diskfree:%" PRId64 "\n",
                dataserverid,fileid,chunkid,diskfree);
    }
};

struct DsHeartBeat:message_base
{
    int32_t servicetype;
    int32_t loadweight;
    int32_t endflag;
    DsHeartBeat()
    {
        memset(this, 0, sizeof(DsHeartBeat));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /HEARTBEAT,loadweight:%d\n",loadweight);
    }
};
#endif
