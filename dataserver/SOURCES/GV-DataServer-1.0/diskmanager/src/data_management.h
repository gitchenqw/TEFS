/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: data_management.h 515 2011-06-17 01:50:58Z duanfei@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   qushan<qushan@taobao.com>
 *      - modify 2009-03-27
 *   zongdai <zongdai@taobao.com>
 *      - modify 2010-04-23
 *
 */
#ifndef TFS_DATASERVER_DATAMANAGEMENT_H_
#define TFS_DATASERVER_DATAMANAGEMENT_H_

#include <vector>
#include <map>
#include <list>
#include "logic_block.h"
#include "dataserver_define.h"
#include "common/parameter.h"

namespace tfs
{
  namespace dataserver
  {

    class DataManagement
    {
      public:
        DataManagement();
        ~DataManagement();

      public:
        void set_file_number(const uint64_t file_number);
        int init_block_files(const common::FileSystemParameter& fs_param);
        void get_ds_filesystem_info(int32_t& block_count, int64_t& use_capacity, int64_t& total_capacity);
        int get_all_logic_block(std::list<LogicBlock*>& logic_block_list);
        int64_t get_all_logic_block_size();

        int write_data(const common::WriteDataInfo& write_info, const char* data_buffer);
        int erase_data_file(const uint64_t file_number);
        int close_write_file(const common::CloseFileInfo& close_file_info, int32_t& write_file_size);
        int read_raw_data(uint64_t fileid,uint32_t chunkid, int32_t read_offset, int32_t& real_read_len, char* tmpDataBuffer);

        int batch_new_block(const common::VINT64* new_blocks);
        int batch_remove_block(const common::VINT64* remove_blocks);

        int query_bit_map(const int32_t query_type, char** tmp_data_buffer, int32_t& bit_map_len, int32_t& set_count);

        int query_block_status(const int32_t query_type, common::VSTRING& block_ids, std::map<std::string, std::vector<
            uint32_t> >& logic_2_physic_blocks);
        int get_block_info(uint64_t fileid,uint32_t chunkid,int32_t& block_size,int32_t& visit_count);

        int get_visit_sorted_blockids(std::vector<LogicBlock*>& block_ptrs);

        int new_single_block(uint64_t fileid,uint32_t chunkid);
        int del_single_block(uint64_t fileid,uint32_t chunkid);
        int get_block_curr_size(uint64_t fileid,uint32_t chunkid, int32_t& size);
        int write_raw_data(uint64_t fileid,uint32_t chunkid, const int32_t data_offset, const int32_t msg_len,
            const char* data_buffer);
     
        int add_new_expire_block(const common::VSTRING* expire_block_ids, const common::VSTRING* remove_block_ids,
            const common::VSTRING* new_block_ids);

        //gc thread
        int gc_data_file();
        int remove_data_file();
        
    public:
        int  show_block_status();
        void show_ds_filesystem_info();
    
      private:
        DISALLOW_COPY_AND_ASSIGN(DataManagement);

        uint64_t file_number_;          // file id
        common::Mutex data_file_mutex_;         // datafile mutex
        DataFileMap data_file_map_;      // datafile map

        //gc datafile
        int32_t last_gc_data_file_time_; // last datafile gc time
        common::RWLock block_rw_lock_;   // block layer read-write lock
    };

    struct visit_count_sort
    {
        bool operator()(const LogicBlock *x, const LogicBlock *y) const
        {
          return (x->get_visit_count() > y->get_visit_count());
        }
    };
  }
}
#endif //TFS_DATASERVER_DATAMANAGEMENT_H_
