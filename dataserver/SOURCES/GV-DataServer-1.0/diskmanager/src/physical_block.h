/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: physical_block.h 552 2011-06-24 08:44:50Z duanfei@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_DATASERVER_PHYSICALBLOCK_H_
#define TFS_DATASERVER_PHYSICALBLOCK_H_

#include <string>
#include "file_op.h"
#include "dataserver_define.h"
//#include "common/config.h"
#include "common/config_item.h"

namespace tfs
{
  using namespace common;
  namespace dataserver
  {
    class PhysicalBlock
    {
      public:
        PhysicalBlock(const uint32_t physical_block_id, const std::string& mount_path, const int32_t block_length,
            const BlockType block_type);
        ~PhysicalBlock();

      public:
        int clear_block_prefix();
        void set_block_prefix(uint64_t fileid,uint32_t chunkid, const uint32_t prev_physic_blockid,
            const uint32_t next_physical_blockid);

        int load_block_prefix();
        int dump_block_prefix();
        void get_block_prefix(BlockPrefix& block_prefix);

        int pread_data(char* buf, const int32_t nbytes, const int32_t offset);
        int pwrite_data(const char* buf, const int32_t nbytes, const int32_t offset);

        inline int32_t get_total_data_len() const
        {
          return total_data_len_;
        }

        inline uint32_t get_physic_block_id() const
        {
          return physical_block_id_;
        }

        inline void set_next_block(const uint32_t next_physical_block_id)
        {
          block_prefix_.next_physic_blockid_ = next_physical_block_id;
          return;
        }

        inline BlockPrefix* get_block_prefix()
        {
          return &block_prefix_;
        }

      private:
        PhysicalBlock();
        DISALLOW_COPY_AND_ASSIGN(PhysicalBlock);

        uint32_t physical_block_id_; // physical block id

        int32_t data_start_; // the data start offset of this block file
        int32_t total_data_len_; // total data size
        BlockPrefix block_prefix_; // block meta info prefix
        FileOperation* file_op_;   // file operation handle
    };

  }
}
#endif //TFS_DATASERVER_PHYSICALBLOCK_H_
