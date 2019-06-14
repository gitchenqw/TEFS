/*
 * (C) 20014-2016 GVTV Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: ClientMessage.cpp  2015-02-11 02:48:00Z  $
 *
 * Authors:
 *      yushuai 
 *      - initial release
 *
 */
#include "ClientMessage.h"

int ClientGet::deserialize(const char* data, const int64_t data_len, int64_t& pos)
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
    iret = Serialization::get_int32(data, data_len, pos, &start);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &end);
  }
  return iret;
}

int ClientGet::serialize(char* data, const int64_t data_len, int64_t& pos) const
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
    iret = Serialization::set_int32(data, data_len, pos, start);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::set_int32(data, data_len, pos, end);
  }
  return iret;
}

int ClientPut::deserialize(const char* data, const int64_t data_len, int64_t& pos)
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
    iret = Serialization::get_int32(data, data_len, pos, &start);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &end);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &backup_dataservernum);
  }
  for(int i = 0;i < backup_dataservernum;i++)
  {
      if (GV_SUCCESS == iret)
      {
#if IPV6            // need to modify
        iret = Serialization::get_bytes(data, data_len, pos, &address[i].ip,16);
#else
        iret = Serialization::get_int32(data, data_len, pos, (int32_t*)address[i].ip);
        pos += 12;
#endif
      }
      if (GV_SUCCESS == iret)
      {
        iret = Serialization::get_int32(data, data_len, pos, &address[i].port);
      }
  } 
  return iret;
}

int ClientPut::deserializehead(const char* data, const int64_t data_len, int64_t& pos)
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
    iret = Serialization::get_int32(data, data_len, pos, &start);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &end);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &backup_dataservernum);
  }
  return iret;
}

int ClientPut::serialize(char* data, const int64_t data_len, int64_t& pos) const
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
    iret = Serialization::set_int32(data, data_len, pos, start);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::set_int32(data, data_len, pos, end);
  }
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::set_int32(data, data_len, pos, backup_dataservernum);
  }
  for(int i = 0;i < backup_dataservernum;i++)
  {
      if (GV_SUCCESS == iret)
      {
#if IPV6
        int addresslen = 16;
        iret = Serialization::set_bytes(data, data_len, pos, address[i].ip,addresslen);
#else
        iret = Serialization::set_int32(data, data_len, pos, *(int32_t*)address[i].ip);
        pos += 12;
#endif
      }
      if (GV_SUCCESS == iret)
      {
        iret = Serialization::set_int32(data, data_len, pos, address[i].port);
      }
  }
  return iret;
}

int ClientPutRes::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &retcode);
  }
  return iret;
}

int ClientPutRes::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, retcode);
    }
    return iret;
}


