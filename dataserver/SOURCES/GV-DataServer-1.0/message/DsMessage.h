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
#ifndef GV_DS_MESSAGE_H_
#define GV_DS_MESSAGE_H_
#include "Message.h"

struct DsPush:message_base
{
    int64_t fileid;
    int32_t chunkid;
    int32_t blocksize;
    DsPush()
    {
        memset(this, 0, sizeof(DsPush));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  Method /PUSH,blockid:<%" PRId64 ",%d>\n",fileid,chunkid);
        printf("  size:%d\n",blocksize);
    }
};

struct DsPushRes:message_base
{
    int32_t retcode;
    DsPushRes()
    {
        memset(this, 0, sizeof(DsPush));
    }
    int serialize(char* data, const int64_t data_len, int64_t& pos) const;
    int deserialize(const char* data, const int64_t data_len, int64_t& pos);
    void dump()
    {
        message_base::dump();
        printf("  return code %d,",retcode);
    }
};

#endif
