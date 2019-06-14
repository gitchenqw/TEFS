/*
 * (C) 20014-2016 GVTV Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: DsMessage.cpp  2015-03-5 02:38:00Z  $
 *
 * Authors:
 *      yushuai 
 *      - initial release
 *
 */
#include "NsMessage.h"

int DsRegister::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &servicetype);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &dataserverid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &diskfree);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &diskcapacity);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &port);
  }
  return iret;
}

int DsRegister::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, servicetype);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, dataserverid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, diskfree);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, diskcapacity);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, port);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

int DsRegisterRes::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &retcode);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &blocksize);
  }
  return iret;
}

int DsRegisterRes::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, retcode);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, blocksize);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

int DeleteCommand::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &fileid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &chunkid);
  }
  return iret;
}

int DeleteCommand::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, fileid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, chunkid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

int DeleteCommandRes::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &fileid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &chunkid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &diskfree);
  }
  return iret;
}

int DeleteCommandRes::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, fileid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, chunkid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, diskfree);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

int BackupCommand::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &fileid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &chunkid);
  }
  if (GV_SUCCESS == iret)
  {
#if IPV6            // need to modify
    iret = Serialization::get_bytes(data, data_len, pos, &src_address.ip,16);
#else
    iret = Serialization::get_int32(data, data_len, pos, (int32_t*)src_address.ip);
    pos += 12; //pad to 16 bytes
#endif
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &src_address.port);
  }
  if (GV_SUCCESS == iret)
  {
#if IPV6            // need to modify
    iret = Serialization::get_bytes(data, data_len, pos, &dst_address.ip,16);
#else
    iret = Serialization::get_int32(data, data_len, pos, (int32_t*)dst_address.ip);
    pos += 12;
#endif
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &dst_address.port);
  }
  return iret;
}

int BackupCommand::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, fileid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, chunkid);
    }
    if (GV_SUCCESS == iret)
    {
#if IPV6
        iret = Serialization::set_bytes(data, data_len, pos, src_address.ip,addresslen);
#else
        iret = Serialization::set_int32(data, data_len, pos, *(int32_t*)src_address.ip);
        pos += 12;
#endif
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, src_address.port);
    }
    
    if (GV_SUCCESS == iret)
    {
#if IPV6
        iret = Serialization::set_bytes(data, data_len, pos, dst_address.ip,addresslen);
#else
        iret = Serialization::set_int32(data, data_len, pos, *(int32_t*)dst_address.ip);
        pos += 12;
#endif
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, dst_address.port);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

int BackupCommandRes::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &dataserverid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &fileid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &chunkid);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int64(data, data_len, pos, &diskfree);
  }
  return iret;
}

int BackupCommandRes::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, dataserverid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, fileid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, chunkid);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int64(data, data_len, pos, diskfree);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

int DsHeartBeat::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
    int32_t iret = message_base::deserialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::get_int32(data, data_len, pos, &servicetype);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::get_int32(data, data_len, pos, &loadweight);
    }
    return iret;
}

int DsHeartBeat::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, servicetype);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, loadweight);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, endflag);
    }
    return iret;
}

