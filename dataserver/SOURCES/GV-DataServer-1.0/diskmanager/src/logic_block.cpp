/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: logic_block.cpp 563 2011-07-07 01:28:35Z nayan@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   zhuhui <zhuhui_a.pt@taobao.com>
 *      - modify 2010-04-23
 *
 */
#include "blockfile_manager.h"
#include <stdio.h>
#include "logic_block.h"

namespace tfs
{
  namespace dataserver
  {
    using namespace common;
    using namespace std;

    LogicBlock::LogicBlock(uint64_t fileid,uint32_t chunkid) :
        fileid_(fileid), chunkid_(chunkid),avail_data_size_(0), visit_count_(0), last_update_(time(NULL)),
        last_abnorm_time_(0)
    {
        data_handle_ = new DataHandle(this);
        physical_block_list_.clear();
    }

    LogicBlock::~LogicBlock()
    {
      if (data_handle_)
      {
        delete data_handle_;
        data_handle_ = NULL;
      }
    }

    int LogicBlock::delete_block_file()
    {
      list<PhysicalBlock*>::iterator lit = physical_block_list_.begin();
      for (; lit != physical_block_list_.end(); ++lit)
      {
        (*lit)->clear_block_prefix();
      }
      return TFS_SUCCESS;
    }

    void LogicBlock::add_physic_block(PhysicalBlock* physic_block)
    {
      physical_block_list_.push_back(physic_block);
      avail_data_size_ += physic_block->get_total_data_len();
      return;
    }
    
    int LogicBlock::read_raw_data(char* buf, int32_t& nbytes, const int32_t offset)
    {
        if (NULL == buf)
        {
            return EXIT_POINTER_NULL;
        }
      if(avail_data_size_ < offset)
      {    
          TBSYS_LOG(TFWARN, "fileid: %Lu,chunkid:%u, read data out of range, offset:%d,available size: %d,",
            fileid_,chunkid_, offset,avail_data_size_);
          return EXIT_FAILURE;
      }
      if(avail_data_size_ < offset+nbytes)
      {    
          nbytes = avail_data_size_ - offset;
          TBSYS_LOG(TFDEBUG, "fileid: %Lu,chunkid:%u read file end, offset:%d,available size: %d,",
            fileid_,chunkid_, offset,avail_data_size_);
      }
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      int ret = data_handle_->read_segment_data(buf, nbytes, offset);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, 
            "fileid: %Lu,chunkid:%u read data batch fail, size: %d, offset:%d, ret: %d",
            fileid_,chunkid_,nbytes, offset, ret);
        return ret;
      }

      return TFS_SUCCESS;
    }

    int LogicBlock::write_raw_data(const char* buf, const int32_t nbytes, const int32_t offset)
    {
      if (NULL == buf)
      {
        return EXIT_POINTER_NULL;
      }
      //limit the nbytes of per write
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      int ret = extend_block(nbytes, offset);
      if (TFS_SUCCESS != ret)
        return ret;

      ret = data_handle_->write_segment_data(buf, nbytes, offset);
        return ret;
    }
    
    int LogicBlock::extend_block(const int32_t size, const int32_t offset)
    {
      int32_t retry_times = MAX_EXTEND_TIMES;
      // extend retry_times extend block in one call
      while (retry_times)
      {
        if (offset + size > avail_data_size_) // need extend block
        {
#if EXTEND_SUPPORT 
          TBSYS_LOG(TFDEBUG,
              "blockid<%Lu,%u> need ext block. offset: %d, datalen: %d, availsize: %d,retry: %d",
              fileid_,chunkid_,offset, size, avail_data_size_, retry_times);
#else
          TBSYS_LOG(TFERROR,
              "blockid<%Lu,%u> need ext block. offset: %d, datalen: %d, availsize: %d,but EXTEND is not supported",
              fileid_,chunkid_,offset, size, avail_data_size_);
           return TFS_ERROR;
#endif
          uint32_t physical_ext_blockid = 0;
          uint32_t physical_blockid = 0;
          // get the last prev block id of this logic block
          std::list<PhysicalBlock*>* physcial_blk_list = &physical_block_list_;
          if (0 == physcial_blk_list->size())
          {
            TBSYS_LOG(TFERROR,"blockid<%Lu,%u> physical block list is empty!", fileid_,chunkid_);
            return EXIT_PHYSICALBLOCK_NUM_ERROR;
          }

          physical_blockid = physcial_blk_list->back()->get_physic_block_id();
          // new one ext block
          PhysicalBlock* tmp_physic_block = NULL;
          
          int ret = BlockFileManager::get_instance()->new_ext_block(fileid_,chunkid_,physical_blockid,
              physical_ext_blockid, &tmp_physic_block);
    
          if (TFS_SUCCESS != ret)
            return ret;

          // update avail size(extend size)
          physical_block_list_.push_back(tmp_physic_block);
          avail_data_size_ += tmp_physic_block->get_total_data_len();
        }
        else                    // no extend block need
        {
          break;
        }
        --retry_times;
      }

      if (0 == retry_times)
      {
        TBSYS_LOG(TFERROR, "blockid<%Lu,%u> extend block too much!", fileid_,chunkid_);
        return EXIT_PHYSICALBLOCK_NUM_ERROR;
      }
      return TFS_SUCCESS;
    }

  }
}
