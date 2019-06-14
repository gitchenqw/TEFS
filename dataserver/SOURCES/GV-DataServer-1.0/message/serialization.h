/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * hton and ntoh
 *
 */
#ifndef GV_COMMON_SERIALIZATION_H_
#define GV_COMMON_SERIALIZATION_H_

#define GV_SUCCESS 0
#define GV_ERROR   -1
#include <stdint.h>
#include <stdlib.h>
#include "string.h"

static const int8_t INT8_SIZE = 1;
static const int8_t INT16_SIZE = 2;
static const int8_t INT_SIZE = 4;
static const int8_t INT64_SIZE = 8;

struct Serialization
{
  static int get_int8(const char* data, const int64_t data_len, int64_t& pos, int8_t* value)
  {
    int32_t iret = NULL != value && NULL != data && data_len - pos >= INT8_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      *value = data[pos++];
    }
    return iret;
  }

  static int get_int16(const char* data, const int64_t data_len, int64_t& pos, int16_t* value)
  {
    int32_t iret = NULL != value && NULL != data && data_len - pos >= INT16_SIZE  &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
#if BIGENDIAN
      int64_t tmp = pos += INT16_SIZE;
      *value = static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
#else
      int64_t tmp = pos;
      pos += INT16_SIZE;
      *value = static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
#endif
    }
    return iret;
  }

  static int get_int32(const char* data, const int64_t data_len, int64_t& pos, int32_t* value)
  {
    int32_t iret = NULL != value && NULL != data && data_len - pos >= INT_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
#if BIGENDIAN
      int64_t tmp = pos += INT_SIZE;
      *value = static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
#else
      int64_t tmp = pos;
      pos += INT_SIZE;
      *value = static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
#endif
    }
    return iret;
  }

  static int get_int64(const char* data, const int64_t data_len, int64_t& pos, int64_t* value)
  {
    int32_t iret = NULL != value && NULL != data && data_len - pos >= INT64_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
#if BIGENDIAN
      int64_t tmp = pos += INT64_SIZE;
      *value = static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[--tmp]);
#else
      int64_t tmp = pos;
      pos += INT64_SIZE;
      *value = static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
      *value <<= 8;
      *value |= static_cast<unsigned char>(data[tmp++]);
#endif
    }
    return iret;
  }

  static int get_bytes(const char* data, const int64_t data_len, int64_t& pos, void* buf, const int64_t buf_length)
  {
    int32_t iret = NULL != data && NULL != buf && buf_length > 0 && data_len - pos >= buf_length &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      memcpy(buf, (data+pos), buf_length);
      pos += buf_length;
    }
    return iret;
  }

  static int get_string(const char* data, const int64_t data_len, int64_t& pos, const int64_t str_buf_length, char* str, int64_t& real_str_buf_length)
  {
    int32_t iret = NULL != data &&  data_len - pos >= INT_SIZE  &&  pos >= 0  && NULL != str && str_buf_length > 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      str[0] = '\0';
      real_str_buf_length = 0;
      int32_t length  = 0;
      iret = get_int32(data, data_len, pos, &length);
      if (GV_SUCCESS == iret)
      {
        if (length > 0)
        {
          iret = length <= str_buf_length ? GV_SUCCESS : GV_ERROR;
          if (GV_SUCCESS == iret)
          {
            iret = data_len - pos >= length ? GV_SUCCESS : GV_ERROR;
            if (GV_SUCCESS == iret)
            {
              memcpy(str, (data+pos), length);
              pos += length;
              real_str_buf_length = length - 1;
            }
          }
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int get_vint8(const char* data, const int64_t data_len, int64_t& pos, T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      int32_t length = 0;
      iret =  Serialization::get_int32(data, data_len, pos, &length);
      if (GV_SUCCESS == iret
          && length > 0)
      {
        iret = data_len - pos >= length * INT8_SIZE ? GV_SUCCESS : GV_ERROR;
        if (GV_SUCCESS == iret)
        {
          int8_t tmp = 0;
          for (int32_t i = 0; i < length; ++i)
          {
            iret = Serialization::get_int8(data, data_len, pos, &tmp);
            if (GV_SUCCESS == iret)
              value.push_back(tmp);
            else
              break;
          }
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int get_vint16(const char* data, const int64_t data_len, int64_t& pos, T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      int32_t length = 0;
      iret =  Serialization::get_int32(data, data_len, pos, &length);
      if (GV_SUCCESS == iret
          && length > 0)
      {
        iret = data_len - pos >= length * INT16_SIZE ? GV_SUCCESS : GV_ERROR;
        if (GV_SUCCESS == iret)
        {
          int16_t tmp = 0;
          for (int32_t i = 0; i < length; ++i)
          {
            iret = Serialization::get_int16(data, data_len, pos, &tmp);
            if (GV_SUCCESS == iret)
              value.push_back(tmp);
            else
              break;
          }
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int get_vint32(const char* data, const int64_t data_len, int64_t& pos, T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      int32_t length = 0;
      iret =  Serialization::get_int32(data, data_len, pos, &length);
      if (GV_SUCCESS == iret
          && length > 0)
      {
        iret = data_len - pos >= length * INT_SIZE ? GV_SUCCESS : GV_ERROR;
        if (GV_SUCCESS == iret)
        {
          int32_t tmp = 0;
          for (int32_t i = 0; i < length; ++i)
          {
            iret = Serialization::get_int32(data, data_len, pos, &tmp);
            if (GV_SUCCESS == iret)
              value.push_back(tmp);
            else
              break;
          }
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int get_vint64(const char* data, const int64_t data_len, int64_t& pos, T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      int32_t length = 0;
      iret =  Serialization::get_int32(data, data_len, pos, &length);
      if (GV_SUCCESS == iret
          && length > 0)
      {
        iret = data_len - pos >= length * INT64_SIZE ? GV_SUCCESS : GV_ERROR;
        if (GV_SUCCESS == iret)
        {
          int64_t tmp = 0;
          for (int32_t i = 0; i < length; ++i)
          {
            iret = Serialization::get_int64(data, data_len, pos, &tmp);
            if (GV_SUCCESS == iret)
              value.push_back(tmp);
            else
              break;
          }
        }
      }
    }
    return iret;
  }

  static int set_int8(char* data, const int64_t data_len, int64_t& pos, const int8_t value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT8_SIZE  &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      data[pos++] = value;
    }
    return iret;
  }

  static int set_int16(char* data, const int64_t data_len, int64_t& pos, const int16_t value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT16_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
#if BIGENDIAN        
    if (GV_SUCCESS == iret)
    {
      data[pos++] = value & 0xFF;
      data[pos++]= (value >> 8) & 0xFF;
    }
#else
    if (GV_SUCCESS == iret)
    {
      data[pos++] = (value >> 8) & 0xFF;
      data[pos++]= value & 0xFF;
    }
#endif
    return iret;
  }

  static int set_int32(char* data, const int64_t data_len, int64_t& pos, const int32_t value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
#if BIGENDIAN 
    if (GV_SUCCESS == iret)
    {
      data[pos++] =  value & 0xFF;
      data[pos++] =  (value >> 8) & 0xFF;
      data[pos++] =  (value >> 16) & 0xFF;
      data[pos++] =  (value >> 24) & 0xFF;
    }
#else
    if (GV_SUCCESS == iret)
    {
      data[pos++] =  (value >> 24) & 0xFF;
      data[pos++] =  (value >> 16) & 0xFF;
      data[pos++] =  (value >> 8) & 0xFF;
      data[pos++] =  value & 0xFF;
    }
#endif
    return iret;
  }

  static int set_int64(char* data, const int64_t data_len, int64_t& pos, const int64_t value)
  {
    int32_t iret = NULL != data && data_len - pos >= INT64_SIZE &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
#if BIGENDIAN 
    if (GV_SUCCESS == iret)
    {
      data[pos++] =  (value) & 0xFF;
      data[pos++] =  (value >> 8) & 0xFF;
      data[pos++] =  (value >> 16) & 0xFF;
      data[pos++] =  (value >> 24) & 0xFF;
      data[pos++] =  (value >> 32) & 0xFF;
      data[pos++] =  (value >> 40) & 0xFF;
      data[pos++] =  (value >> 48) & 0xFF;
      data[pos++] =  (value >> 56) & 0xFF;
    }
#else
    {
      data[pos++] =  (value >> 56) & 0xFF;
      data[pos++] =  (value >> 48) & 0xFF;
      data[pos++] =  (value >> 40) & 0xFF;
      data[pos++] =  (value >> 32) & 0xFF;
      data[pos++] =  (value >> 24) & 0xFF;
      data[pos++] =  (value >> 16) & 0xFF;
      data[pos++] =  (value >> 8) & 0xFF;
      data[pos++] =  (value) & 0xFF;
    }
#endif
    return iret;
  }


  static int set_string(char* data, const int64_t data_len, int64_t& pos, const char* str)
  {
    int32_t iret = NULL != data &&  pos < data_len &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      int64_t length = NULL == str ? 0 : strlen(str) == 0 ? 0 : strlen(str) + 1;/** include '\0'**/
      iret = data_len - pos >= (length + INT_SIZE) ? GV_SUCCESS : GV_ERROR;
      if (GV_SUCCESS == iret)
      {
        iret = set_int32(data, data_len, pos, length);
        if (GV_SUCCESS == iret)
        {
          if (length > 0)
          {
            memcpy((data+pos), str, length - 1);
            pos += length;
            data[pos - 1] = '\0';
          }
        }
      }
    }
    return iret;
  }

  static int set_bytes(char* data, const int64_t data_len, int64_t& pos, const void* buf, const int64_t buf_length)
  {
    int32_t iret = NULL != data && buf_length > 0 && NULL != buf && data_len - pos >= buf_length &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      memcpy((data+pos), buf, buf_length);
      pos += buf_length;
    }
    return iret;
  }

  template <typename T>
  static int set_vint8(char* data, const int64_t data_len, int64_t& pos, const T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= get_vint8_length(value) &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      iret = Serialization::set_int32(data, data_len, pos, value.size());
      if (GV_SUCCESS == iret)
      {
        typename T::const_iterator iter = value.begin();
        for (; iter != value.end(); ++iter)
        {
          iret = Serialization::set_int8(data, data_len, pos, (*iter));
          if (GV_SUCCESS != iret)
            break;
        }
      }
    }
    return iret;
  }
  template <typename T>
  static int set_vint16(char* data, const int64_t data_len, int64_t& pos, const T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= get_vint16_length(value) &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      iret = Serialization::set_int32(data, data_len, pos, value.size());
      if (GV_SUCCESS == iret)
      {
        typename T::const_iterator iter = value.begin();
        for (; iter != value.end(); ++iter)
        {
          iret = Serialization::set_int16(data, data_len, pos, (*iter));
          if (GV_SUCCESS != iret)
            break;
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int set_vint32(char* data, const int64_t data_len, int64_t& pos, const T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= get_vint32_length(value) &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      iret = Serialization::set_int32(data, data_len, pos, value.size());
      if (GV_SUCCESS == iret)
      {
        typename T::const_iterator iter = value.begin();
        for (; iter != value.end(); ++iter)
        {
          iret = Serialization::set_int32(data, data_len, pos, (*iter));
          if (GV_SUCCESS != iret)
            break;
        }
      }
    }
    return iret;
  }
  template <typename T>
  static int set_vint64(char* data, const int64_t data_len, int64_t& pos, const T& value)
  {
    int32_t iret = NULL != data && data_len - pos >= get_vint64_length(value) &&  pos >= 0 ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
      iret = Serialization::set_int32(data, data_len, pos, value.size());
      if (GV_SUCCESS == iret)
      {
        typename T::const_iterator iter = value.begin();
        for (; iter != value.end(); ++iter)
        {
          iret = Serialization::set_int64(data, data_len, pos, (*iter));
          if (GV_SUCCESS != iret)
            break;
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int serialize_list(char* data, const int64_t data_len, int64_t& pos, const T& value)
  {
    int32_t iret = set_int32(data, data_len, pos, value.size());
    if (GV_SUCCESS == iret)
    {
      typename T::const_iterator iter = value.begin();
      for (; iter != value.end(); ++iter)
      {
        iret = (*iter).serialize(data, data_len, pos);
        if (GV_SUCCESS != iret)
        {
          break;
        }
      }
    }
    return iret;
  }

  template <typename T>
  static int deserialize_list(const char* data, const int64_t data_len, int64_t& pos, T& value)
  {
    int32_t len = 0;
    int32_t iret = Serialization::get_int32(data, data_len, pos, &len);
    if (GV_SUCCESS == iret)
    {
      for (int32_t i = 0; i < len; ++i)
      {
        typename T::value_type tmp;
        iret = tmp.deserialize(data, data_len, pos);
        if (GV_SUCCESS != iret)
          break;
        else
          value.push_back(tmp);
      }
    }
    return iret;
  }

  template <typename T>
  static int64_t get_list_length(const T& value)
  {
    int64_t len = INT_SIZE;
    typename T::const_iterator iter = value.begin();
    for (; iter != value.end(); ++iter)
    {
      len += iter->length();
    }
    return len;
  }

  template <typename T>
  static int serialize_kv(char* data, const int64_t data_len, int64_t& pos, const T& value)
  {
    //TODO
    return GV_SUCCESS;
  }

  template <typename T>
  static int deserialize_kv(const char* data, const int64_t data_len, int64_t& pos, T& value)
  {
    //TODO
    return GV_SUCCESS;
  }

  template <typename T>
  static int64_t get_kv_length(const T& value)
  {
    //TODO
    return GV_SUCCESS;
  }
};
#endif /** TFS_COMMON_SERIALIZATION_H_ */

