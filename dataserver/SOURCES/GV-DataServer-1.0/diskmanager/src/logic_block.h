/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: logic_block.h 552 2011-06-24 08:44:50Z duanfei@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   zhuhui <zhuhui_a.pt@taobao.com>
 *      - modify 2010-04-23
 *
 */
#ifndef TFS_DATASERVER_LOGICALBLOCK_H_
#define TFS_DATASERVER_LOGICALBLOCK_H_

#include <string>
#include <list>
#include "dataserver_define.h"
#include "physical_block.h"
#include "data_handle.h"
#include "common/lock.h"
#include "common/error_msg.h"
#include "data_file.h"

namespace tfs
{
  namespace dataserver
  {
    class DataHandle;
    class LogicBlock
    {
      public:
        LogicBlock(uint64_t fileid,uint32_t chunkid, const uint32_t main_blk_key, const std::string& base_path);
        LogicBlock(uint64_t fileid,uint32_t chunkid);

        ~LogicBlock();

        int set_block_id(uint64_t fileid,uint32_t chunkid)
        {
          fileid_ = fileid;
          chunkid_ = chunkid;
          return common::TFS_SUCCESS;
        }
        
        int delete_block_file();
        void add_physic_block(PhysicalBlock* physic_block);
        
        int read_raw_data(char* buf, int32_t& nbyte, const int32_t offset);
        int write_raw_data(const char* buf, const int32_t nbytes, const int32_t offset);
        int close_write_file(const uint64_t inner_file_id, DataFile* datafile, const uint32_t crc);
      
        std::list<PhysicalBlock*>* get_physic_block_list()
        {
          return &physical_block_list_;
        }
        int32_t get_logic_block_size()
        {
            return avail_data_size_;
        }

        uint64_t file_id() 
        {
          return fileid_;
        }
        
        uint32_t chunk_id() 
        {
          return chunkid_;
        }
        

        int32_t get_visit_count() const
        {
          return visit_count_;
        }

        void add_visit_count()
        {
          ++visit_count_;
        }

        void set_last_update(time_t time)
        {
          last_update_ = time;
        }

        time_t get_last_update() const
        {
          return last_update_;
        }

        void rlock()
        {
          rw_lock_.rdlock();
        }
        void wlock()
        {
          rw_lock_.wrlock();
        }
        void unlock()
        {
          rw_lock_.unlock();
        }

        void set_last_abnorm(time_t time)
        {
          last_abnorm_time_ = time;
        }
        time_t get_last_abnorm() const
        {
          return last_abnorm_time_;
        }

      private:
        int extend_block(const int32_t size, const int32_t offset);

      private:
        DISALLOW_COPY_AND_ASSIGN(LogicBlock);
        static const int32_t MAX_EXTEND_TIMES = 30;

    protected:
        uint64_t fileid_;
        uint32_t chunkid_;
        int32_t avail_data_size_; // the data space of this logic block
        int32_t visit_count_;     // accumlating visit count
        time_t last_update_;      // last update time
        time_t last_abnorm_time_; // last abnormal time
 //       BlockStatus block_health_status; // block status info

        DataHandle* data_handle_;   // data operation handle
        //IndexHandle* index_handle_; // associate index handle
        std::list<PhysicalBlock*> physical_block_list_; // the physical block list of this logic block
        common::RWLock rw_lock_;   // read-write lock
    };
  }
}

#endif //TFS_DATASERVER_LOGICALBLOCK_H_
