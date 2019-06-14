/*
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *      - initial release
 *
 */
#ifndef GV_CLIENT_MESSAGE_H_
#define GV_CLIENT_MESSAGE_H_
#include "Message.h"

struct ClientPut:message_base
{
    int64_t fileid;
    int32_t chunkid;
    int32_t start;
    int32_t end;
    int32_t backup_dataservernum;
    address_t address[0];
    ClientPut()
    {
        memset(this, 0, sizeof(ClientPut));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    int deserializehead(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
#if IPV6
        int domain = AF_INET6;
        int addresslen = sizeof(struct in6_addr);
   ```` char str[INET6_ADDRSTRLEN];
          
#else 
        int domain = AF_INET;
        int addresslen = INET_ADDRSTRLEN;
        char str[INET_ADDRSTRLEN];
#endif
        
        printf("  Method /PUT,blockid:<%" PRId64 ",%d>,",fileid,chunkid);
        printf("  range:%d-%d\n",start,end);
        for(int i = 0;i < backup_dataservernum;i++)
        {
            void* ip;
#if IPV6
            
#else
            int32_t ip4 = htonl(*(int32_t*)address[i].ip);
            ip = &ip4;
#endif
            if(inet_ntop(domain, ip, str, addresslen) != NULL) 
            {
                printf("  backup address <%s:%u>",str,address[i].port);
            }
            else
            {
                printf("  ip address is illegal");
                break;
            }
        }
        printf("\n");
    }
};

struct ClientGet:message_base
{
    int64_t fileid;
    int32_t chunkid;
    int32_t start;
    int32_t end;
    ClientGet()
    {
        memset(this, 0, sizeof(ClientGet));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /GET,blockid:<%" PRId64 ",%d>\n",fileid,chunkid);
        printf("  range:%d-%d\n",start,end);
    }
};

struct ClientPutRes:message_base
{
    int32_t retcode;
    ClientPutRes()
    {
        memset(this, 0, sizeof(ClientPutRes));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  return code %d\n",retcode);
    }
};

struct ClientGetRes:ClientPutRes
{
};

#endif
