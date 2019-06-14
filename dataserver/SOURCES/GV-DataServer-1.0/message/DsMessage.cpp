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
#include "DsMessage.h"

int DsPush::deserialize(const char* data, const int64_t data_len, int64_t& pos)
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
    iret = Serialization::get_int32(data, data_len, pos, &blocksize);
  }
  return iret;
}

int DsPush::serialize(char* data, const int64_t data_len, int64_t& pos) const
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
        iret = Serialization::set_int32(data, data_len, pos, blocksize);
    }
    return iret;
}

int DsPushRes::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
  int32_t iret = message_base::deserialize(data,data_len,pos);
  if (GV_SUCCESS == iret)
  {
    iret = Serialization::get_int32(data, data_len, pos, &retcode);
  }
  return iret;
}

int DsPushRes::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = message_base::serialize(data,data_len,pos);
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, retcode);
    }
    return iret;
}
