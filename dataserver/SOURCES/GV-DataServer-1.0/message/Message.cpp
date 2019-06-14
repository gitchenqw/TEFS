#include "Message.h"
#include "../Pref/defaultpref.h"
int32_t  blockinfo::MaxBlockSize = DEFAULTBLOCKSIZE;
int message_base::deserialize(const char* data, const int64_t data_len, int64_t& pos)
{
    int32_t iret = NULL != data && data_len - pos >= sizeof(message_base) ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::get_int32(data, data_len, pos, &length);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::get_int32(data, data_len, pos, &request_id);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::get_int32(data, data_len, pos, &type);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::get_int32(data, data_len, pos, &encrypt);
    }
    return iret;
}
int message_base::serialize(char* data, const int64_t data_len, int64_t& pos) const
{
    int32_t iret = NULL != data && data_len - pos >= sizeof(message_base) ? GV_SUCCESS : GV_ERROR;
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, length);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, request_id);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, type);
    }
    if (GV_SUCCESS == iret)
    {
        iret = Serialization::set_int32(data, data_len, pos, encrypt);
    }
    return iret;
}