/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: blockfile_manager.cpp 370 2011-05-27 07:18:14Z daoan@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *   qushan<qushan@taobao.com>
 *      - modify 2009-03-27
 *
 */
#include "blockfile_manager.h"
#include "blockfile_format.h"
#include "common/directory_op.h"
#include "tbsys.h"
#include <string.h>

namespace tfs
{
   
  namespace dataserver
  {
    using namespace common;
    using namespace std;
    BlockFileManager::~BlockFileManager()
    {
      tbsys::gDelete(normal_bit_map_);
      tbsys::gDelete(error_bit_map_);
      tbsys::gDelete(super_block_impl_);

      destruct_logic_blocks(C_COMPACT_BLOCK);
      destruct_logic_blocks(C_MAIN_BLOCK);
      destruct_physic_blocks();
    }

    void BlockFileManager::destruct_logic_blocks(const BlockType block_type)
    {
      LogicBlockMap* selected_logic_blocks = NULL;
      if (C_COMPACT_BLOCK == block_type)
      {
        selected_logic_blocks = &compact_logic_blocks_;
      }
      else // base on current types, main Block
      {
        selected_logic_blocks = &logic_blocks_;
      }

      for (LogicBlockMapIter mit = selected_logic_blocks->begin(); mit != selected_logic_blocks->end(); ++mit)
      {
          LogicChunkIter Cit;
          for ( Cit= mit->second.begin(); Cit != mit->second.end(); ++Cit)
                tbsys::gDelete(Cit->second);
          mit->second.clear();
      }
      selected_logic_blocks->clear();
    }

    void BlockFileManager::destruct_physic_blocks()
    {
      for (PhysicalBlockMapIter mit = physcial_blocks_.begin(); mit != physcial_blocks_.end(); ++mit)
      {
        tbsys::gDelete(mit->second);
      }
    }

    int BlockFileManager::format_block_file_system(const FileSystemParameter& fs_param)
    {
      // 1. initialize super block parameter
      int ret = init_super_blk_param(fs_param);
      if (TFS_SUCCESS != ret)
        return ret;

      // 2. create mount directory
      ret = create_fs_dir();
      if (TFS_SUCCESS != ret)
        return ret;

      // 3. create super block file
      ret = create_fs_super_blk();
      if (TFS_SUCCESS != ret)
        return ret;

      // 4. pre-allocate create main block
      ret = create_block(C_MAIN_BLOCK);
      if (TFS_SUCCESS != ret)
        return ret;

      // 5. pre-allocate create extend block
      ret = create_block(C_EXT_BLOCK);
      if (TFS_SUCCESS != ret)
        return ret;

      return TFS_SUCCESS;
    }

    int BlockFileManager::clear_block_file_system(const FileSystemParameter& fs_param)
    {
      bool ret = DirectoryOp::delete_directory_recursively(fs_param.mount_name_.c_str());
      TBSYS_LOG(TFDEBUG, "clear block file system end. mount_point: %s, ret: %d", fs_param.mount_name_.c_str(), ret);
      return ret ? TFS_SUCCESS : TFS_ERROR;
    }

    int BlockFileManager::bootstrap(const FileSystemParameter& fs_param)
    {
      // 1. load super block from super block file
      int ret = load_super_blk(fs_param);
      if (TFS_SUCCESS != ret)
        return ret;

      // 2. load block file, create logic block map associate stuff
      return load_block_file();
    }

    int BlockFileManager::new_block(uint64_t fileid,uint32_t chunkid, uint32_t& physical_block_id,
        const BlockType block_type)
    {
      // 1. write block
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      LogicBlockMap* selected_logic_blocks = NULL;

      // 2. get right logic block id handle
      if (C_COMPACT_BLOCK == block_type)
      {
        selected_logic_blocks = &compact_logic_blocks_;
      }
      else //main block
      {
        selected_logic_blocks = &logic_blocks_;
      }
      
      LogicBlock* lmit = find_logic_block_map(*selected_logic_blocks,fileid,chunkid);
      if (lmit)
      {
        TBSYS_LOG(TFERROR, "logic block<%Lu,%u> has already exist,when new logic block!",fileid,chunkid);
        return EXIT_BLOCK_EXIST_ERROR;
      }

      // 3. find avial physical block
      int ret = find_avail_block(physical_block_id, C_MAIN_BLOCK);
      if (TFS_SUCCESS != ret)
        return ret;

      PhysicalBlockMapIter pmit = physcial_blocks_.find(physical_block_id);
      // oops, same physical block id found
      if (pmit != physcial_blocks_.end())
      {
        TBSYS_LOG(TFERROR, "bitmap and physical blocks conflict. fatal error! physical blockid: %u", physical_block_id);
        assert(false);
      }

      // 4. create physical block
      PhysicalBlock* t_physical_block = new PhysicalBlock(physical_block_id, super_block_.mount_point_,
          BLOCK_RESERVER_LENGTH+super_block_.main_block_size_, C_MAIN_BLOCK);
      t_physical_block->set_block_prefix(fileid,chunkid, 0, 0);
      ret = t_physical_block->dump_block_prefix();
      if (ret)
      {
        TBSYS_LOG(TFERROR, "dump physical block fail. fatal error! pos: %u, file: %s, ret: %d", physical_block_id,
            super_block_.mount_point_, ret);
        tbsys::gDelete(t_physical_block);
        return ret;
      }

      // 5. create logic block
      LogicBlock* t_logic_block = new LogicBlock(fileid,chunkid);
      t_logic_block->add_physic_block(t_physical_block);

      TBSYS_LOG(TFDEBUG,
          "new logic block. logic blockid: <%Lu,%u>, physical blockid: %u, physci prev blockid: %u, physci next blockid: %u",
          fileid,chunkid,physical_block_id, 0, 0);

      bool block_count_modify_flag = false;
      do
      {
        // 6. set normal bitmap: have not serialize to disk
        normal_bit_map_->set(physical_block_id);

        // 7. write normal bitmap
        ret = super_block_impl_->write_bit_map(normal_bit_map_, error_bit_map_);
        if (TFS_SUCCESS != ret)
          break;

        // 8. update and write super block info
        ++super_block_.used_block_count_;
        block_count_modify_flag = true;
        ret = super_block_impl_->write_super_blk(super_block_);
        if (TFS_SUCCESS != ret)
          break;

        // 9. sync to disk
        ret = super_block_impl_->flush_file();
        if (TFS_SUCCESS != ret)
          break;

        // 10. init logic block (create index handle file, etc stuff)

        // 11. insert to associate map
        physcial_blocks_.insert(PhysicalBlockMap::value_type(physical_block_id, t_physical_block));
        ret = insert_logic_block_map(*selected_logic_blocks,t_logic_block);

        TBSYS_LOG(TFDEBUG, "new block success! logic blockid: <%Lu,%u>, physical blockid: %u.",
            fileid ,chunkid,physical_block_id);
      }while (0);

      // oops,new block fail,clean.
      // at this point, step 11 has not arrived. must delete here
      if (ret)
      {
        TBSYS_LOG(TFERROR, "new block fail. logic blockid:<%Lu,%u>. ret: %d",fileid,chunkid, ret);

        rollback_superblock(physical_block_id, block_count_modify_flag);
        t_logic_block->delete_block_file();

        tbsys::gDelete(t_logic_block);
        tbsys::gDelete(t_physical_block);
      }
      return ret;
    }

    int BlockFileManager::del_block(uint64_t fileid,uint32_t chunkid, const BlockType block_type)
    {
      TBSYS_LOG(TFINFO, "delete block! logic blockid: <%Lu,%u>. blocktype: %d", fileid,chunkid,block_type);
      // 1. ...
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      LogicBlock* delete_block = NULL;
      BlockType tmp_block_type = block_type;
      // 2. choose right type block to delete
      delete_block = find_logic_block(fileid, chunkid, tmp_block_type);
      if (NULL == delete_block)
      {
        TBSYS_LOG(TFWARN, "can not find logic blockid: <%Lu,%u>. blocktype: %d when delete block",
                    fileid,chunkid,tmp_block_type);
        return EXIT_NO_LOGICBLOCK_ERROR;
      }

      // 3. logic block layer lock
      delete_block->wlock(); //lock this block
      // 4. get physical block list
      list<PhysicalBlock*>* physic_block_list = delete_block->get_physic_block_list();
      int size = physic_block_list->size();
      if (0 == size)
      {
        delete_block->unlock();
        return EXIT_PHYSICALBLOCK_NUM_ERROR;
      }

      list<PhysicalBlock*>::iterator lit = physic_block_list->begin();
      list<PhysicalBlock*> tmp_physic_block;
      tmp_physic_block.clear();

      // 5. erase(but not clean) physic block from physic map
      for (; lit != physic_block_list->end(); ++lit)
      {
        uint32_t physic_id = (*lit)->get_physic_block_id();
        TBSYS_LOG(TFINFO, "blockid: <%Lu,%u>, del physical block! physic blockid: %u.", fileid,chunkid, physic_id);

        PhysicalBlockMapIter mpit = physcial_blocks_.find(physic_id);
        if (mpit == physcial_blocks_.end())
        {
          TBSYS_LOG(TFERROR, "can not find physical block! physic blockid: %u.", physic_id);
          assert(false);
        }
        else
        {
          if (mpit->second)
          {
            tmp_physic_block.push_back(mpit->second);
          }
          physcial_blocks_.erase(mpit);
        }
        // normal bitmap clear reset
        normal_bit_map_->reset(physic_id);
      }

      // 7. delete logic block from logic block map
      erase_logic_block(fileid,chunkid,tmp_block_type);

      int ret = TFS_SUCCESS;
      do
      {
        // 8. write bitmap
        ret = super_block_impl_->write_bit_map(normal_bit_map_, error_bit_map_);
        if (TFS_SUCCESS != ret)
          break;
        // 9. update and write superblock info
        super_block_.used_block_count_ -= 1;
        super_block_.used_extend_block_count_ -= (size - 1);
        ret = super_block_impl_->write_super_blk(super_block_);
        if (TFS_SUCCESS != ret)
          break;

        // 10. flush suplerblock
        ret = super_block_impl_->flush_file();
      }
      while (0);

      TBSYS_LOG(TFINFO, "blockid: <%Lu,%u>, delete %s!. physical block size: %d, blocktype: %d, ret: %d",
                fileid,chunkid,ret ? "fail" : "success", size, tmp_block_type, ret);

      // 11. clean logic block associate stuff(index handle, physic block) & unlock
      if (delete_block)
      {
        delete_block->delete_block_file();
        delete_block->unlock();
      }
      tbsys::gDelete(delete_block);

      // 12. clean physic block
      for (list<PhysicalBlock*>::iterator lit = tmp_physic_block.begin(); lit != tmp_physic_block.end(); ++lit)
      {
        tbsys::gDelete(*lit);
      }
      return ret;
    }

    int BlockFileManager::new_ext_block(uint64_t fileid,uint32_t chunkid, const uint32_t physical_block_id,
                                        uint32_t& ext_physical_block_id, PhysicalBlock **physic_block)
    {
      // 1. ...
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      int ret = TFS_SUCCESS;
      BlockType block_type;
      if (0 != physical_block_id)
      {
        block_type = C_EXT_BLOCK;
      }
      else
      {
        return EXIT_NO_LOGICBLOCK_ERROR;
      }

      // 2. ...
      if(find_logic_block_map(logic_blocks_,fileid,chunkid) == NULL)
      {
        return EXIT_NO_LOGICBLOCK_ERROR;
      }

      // 3. get available extend physic block
      ret = find_avail_block(ext_physical_block_id, block_type);
      if (TFS_SUCCESS != ret)
        return ret;

      PhysicalBlockMapIter pmit = physcial_blocks_.find(ext_physical_block_id);
      if (pmit != physcial_blocks_.end())
      {
        TBSYS_LOG(TFERROR, "physical block conflict. fatal error! ext physical blockid: %u", ext_physical_block_id);
        assert(false);
      }

      // 4. make sure physical_block_id exist
      pmit = physcial_blocks_.find(physical_block_id);
      if (pmit == physcial_blocks_.end())
      {
        TBSYS_LOG(TFERROR, "can not find physical blockid: %u", physical_block_id);
        assert(false);
      }

      if (NULL == pmit->second)
      {
        TBSYS_LOG(TFERROR, "physical blockid: %u point null", physical_block_id);
        assert(false);
      }

      // 5. create new physic block
      PhysicalBlock* tmp_physical_block = new PhysicalBlock(ext_physical_block_id, super_block_.mount_point_,
                                                            BLOCK_RESERVER_LENGTH+super_block_.extend_block_size_, C_EXT_BLOCK);

      TBSYS_LOG(TFINFO, "new ext block. logic blockid: <%Lu,%u>, prev physical blockid: %u, now physical blockid: %u",
                fileid,chunkid, physical_block_id, ext_physical_block_id);

      bool block_count_modify_flag = false;
      do
      {
        // 6. bitmap set
        normal_bit_map_->set(ext_physical_block_id);

        // 7. write bitmap
        ret = super_block_impl_->write_bit_map(normal_bit_map_, error_bit_map_);
        if (TFS_SUCCESS != ret)
          break;

        // 8. update & write superblock info
        ++super_block_.used_extend_block_count_;
        block_count_modify_flag = true;
        ret = super_block_impl_->write_super_blk(super_block_);
        if (TFS_SUCCESS != ret)
          break;

        // 9. sync to disk
        ret = super_block_impl_->flush_file();
        if (TFS_SUCCESS != ret)
          break;

        // 10. write physical block info to disk
        tmp_physical_block->set_block_prefix(fileid,chunkid,physical_block_id, 0);
        ret = tmp_physical_block->dump_block_prefix();
        if (TFS_SUCCESS != ret)
          break;

        // 11. add to extend block list
        // get previous physcial block and record the next block
        PhysicalBlock* prev_physic_block = pmit->second;
        prev_physic_block->set_next_block(ext_physical_block_id);
        // write prev block info to disk
        ret = prev_physic_block->dump_block_prefix();
        if (TFS_SUCCESS != ret)
          break;

        // 12. insert to physic block map
        physcial_blocks_.insert(PhysicalBlockMap::value_type(ext_physical_block_id, tmp_physical_block));
        (*physic_block) = tmp_physical_block;
      }while (0);

      if (TFS_SUCCESS != ret)   // not arrive step 12
      {
        TBSYS_LOG(TFERROR, "new ext block error! logic blockid: <%Lu,%u>. ret: %d",fileid,chunkid, ret);
        rollback_superblock(ext_physical_block_id, block_count_modify_flag);
        tbsys::gDelete(tmp_physical_block);
      }
      return ret;
    }

    int BlockFileManager::switch_compact_blk(uint64_t fileid,uint32_t chunkid)
    {
        ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
        LogicBlock *cpt_logic_block = find_logic_block_map(compact_logic_blocks_,fileid,chunkid);
        LogicBlock *old_logic_block = find_logic_block_map(logic_blocks_,fileid,chunkid);
        if(!cpt_logic_block||!old_logic_block)
            return EXIT_COMPACT_BLOCK_ERROR;
      // switch
        insert_logic_block_map(logic_blocks_,cpt_logic_block);
        insert_logic_block_map(compact_logic_blocks_,old_logic_block);
        return TFS_SUCCESS;
    }

    int BlockFileManager::expire_compact_blk(const time_t time, std::set<uint64_t>& erase_blocks)
    {
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      erase_blocks.clear();
      int32_t old_compact_block_size = compact_logic_blocks_.size();
      for (LogicBlockMapIter mit = compact_logic_blocks_.begin(); mit != compact_logic_blocks_.end();)
      {
        if (time < 0)
          break;
        for(LogicChunkIter cit = mit->second.begin();cit != mit->second.end();)
        {
            // get last update
            if ((cit->second) && (cit->second->get_last_update() < time))
            {
                TBSYS_LOG(TFINFO, "compact block is expired. blockid:<%Lu,%u>, now: %ld, last update: %ld", 
                mit->first,cit->first,time,cit->second->get_last_update());
                // add to erase block
                erase_blocks.insert(mit->first);
                erase_blocks.insert(cit->first);
            }
        }

        ++mit;
      }

      int32_t new_compact_block_size = compact_logic_blocks_.size() - erase_blocks.size()/2;
      TBSYS_LOG(TFINFO, "compact block map: old: %d, new: %d", old_compact_block_size, new_compact_block_size);
      return TFS_SUCCESS;
    }

    int BlockFileManager::set_error_bitmap(const std::set<uint32_t>& error_blocks)
    {
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      // batch set error block bitmap
      for (std::set<uint32_t>::iterator sit = error_blocks.begin(); sit != error_blocks.end(); ++sit)
      {
        TBSYS_LOG(TFINFO, "set error bitmap, pos: %d", *sit);
        error_bit_map_->set(*sit);
      }
      return TFS_SUCCESS;
    }

    int BlockFileManager::reset_error_bitmap(const std::set<uint32_t>& reset_error_blocks)
    {
      ScopedRWLock scoped_lock(rw_lock_, WRITE_LOCKER);
      // batch reset error block bitmap
      for (std::set<uint32_t>::iterator sit = reset_error_blocks.begin(); sit != reset_error_blocks.end(); ++sit)
      {
        TBSYS_LOG(TFINFO, "reset error bitmap, pos: %d", *sit);
        error_bit_map_->reset(*sit);
      }
      return TFS_SUCCESS;
    }

    LogicBlock* BlockFileManager::get_logic_block(uint64_t fileid,uint32_t chunkid, const BlockType block_type)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      LogicBlockMap* selected_logic_blocks;
      if (C_COMPACT_BLOCK == block_type)
      {
        selected_logic_blocks = &compact_logic_blocks_;
      }
      else if (C_MAIN_BLOCK == block_type)
      {
        selected_logic_blocks = &logic_blocks_;
      }
      else
      {
        return NULL;
      }
      LogicBlock* logicblock = find_logic_block_map(*selected_logic_blocks,fileid,chunkid);
      if(logicblock)
        logicblock->add_visit_count();
      return logicblock;
    }

    int BlockFileManager::get_all_logic_block(std::list<LogicBlock*>& logic_block_list, const BlockType block_type)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      logic_block_list.clear();
      if (C_MAIN_BLOCK == block_type)
      {
        for (LogicBlockMapIter mit = logic_blocks_.begin(); mit != logic_blocks_.end(); ++mit)
        {
            for(LogicChunkIter cit = mit->second.begin();cit != mit->second.end();++cit)
                logic_block_list.push_back(cit->second);
        }
      }
      else                      // compact logic blocks
      {
        for (LogicBlockMapIter mit = compact_logic_blocks_.begin(); mit != compact_logic_blocks_.end(); ++mit)
        {
            for(LogicChunkIter cit = mit->second.begin();cit != mit->second.end();++cit)
                logic_block_list.push_back(cit->second);
        }
      }
      return TFS_SUCCESS;
    }

    int64_t BlockFileManager::get_all_logic_block_size(const BlockType block_type)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      return get_logic_block_size_map(block_type);
    }

    int BlockFileManager::get_logic_block_ids(VSTRING& logic_block_ids, const BlockType block_type)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      logic_block_ids.clear();
      LogicBlockMap* select_block;
      if (C_MAIN_BLOCK == block_type)
      {
          select_block = &logic_blocks_;
      }
      else
      {
         select_block = &compact_logic_blocks_; 
      }
      for (LogicBlockMapIter mit = select_block->begin(); mit != select_block->end(); ++mit)
      {
            for(LogicChunkIter cit = mit->second.begin();cit != mit->second.end();++cit)
            {
                char str[32];
                combine_itoa(mit->first,cit->first,str,10);
                logic_block_ids.push_back((std::string)str);            //get fileid
            }
        }
      return TFS_SUCCESS;
    }

    int BlockFileManager::get_all_physic_block(list<PhysicalBlock*>& physic_block_list)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      physic_block_list.clear();
      for (PhysicalBlockMapIter mit = physcial_blocks_.begin(); mit != physcial_blocks_.end(); ++mit)
      {
        physic_block_list.push_back(mit->second);
      }
      return TFS_SUCCESS;
    }

    int BlockFileManager::query_super_block(SuperBlock& super_block_info)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      super_block_info = super_block_;
      return TFS_SUCCESS;
    }

    int BlockFileManager::query_bit_map(char** bit_map_buffer, int32_t& bit_map_len, int32_t& set_count,
                                        const BitMapType bitmap_type)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      bit_map_len = bit_map_size_;
      *bit_map_buffer = new char[bit_map_size_];
      if (C_ALLOCATE_BLOCK == bitmap_type)
      {
        set_count = normal_bit_map_->get_set_count();
        memcpy(*bit_map_buffer, normal_bit_map_->get_data(), bit_map_size_);
      }
      else
      {
        set_count = error_bit_map_->get_set_count();
        memcpy(*bit_map_buffer, error_bit_map_->get_data(), bit_map_size_);
      }
      return TFS_SUCCESS;
    }
    
    int BlockFileManager::get_bit_map_info(int32_t& bit_map_len, int32_t& normal_set_count,int32_t& error_set_count)
    {
      ScopedRWLock scoped_lock(rw_lock_, READ_LOCKER);
      bit_map_len = bit_map_size_;
      normal_set_count = normal_bit_map_->get_set_count();
      error_set_count = error_bit_map_->get_set_count();
      return TFS_SUCCESS;
    }

    int BlockFileManager::query_approx_block_count(int32_t& block_count) const
    {
      // use super block recorded count, just log confict
        TBSYS_LOG(TFDEBUG, "total file: %u, super main block: %u", logic_blocks_.size(),
                  super_block_.used_block_count_);

      // modify ratio
      int32_t approx_block_count = static_cast<int32_t> (super_block_.used_extend_block_count_
                                                         * super_block_.block_type_ratio_);

      // hardcode 1.1 approx radio
      if (super_block_.used_block_count_ * 1.1 < approx_block_count)
      {
        TBSYS_LOG(TFDEBUG, "used mainblock: %u, used approx block: %u", super_block_.used_block_count_,
                  approx_block_count);
        block_count = approx_block_count;
        return TFS_SUCCESS;
      }

      block_count = super_block_.used_block_count_;
      return TFS_SUCCESS;
    }

    int BlockFileManager::query_space(int64_t& used_bytes, int64_t& total_bytes) const
    {
      int64_t used_main_bytes = static_cast<int64_t> (super_block_.used_block_count_)
        * static_cast<int64_t> (super_block_.main_block_size_) + static_cast<int64_t> (super_block_.used_block_count_
                                                                                       / super_block_.block_type_ratio_) * static_cast<int64_t> (super_block_.extend_block_size_);

      int64_t used_ext_bytes = static_cast<int64_t> (super_block_.used_extend_block_count_)
        * static_cast<int64_t> (super_block_.extend_block_size_)
        + static_cast<int64_t> (super_block_.used_extend_block_count_ * super_block_.block_type_ratio_)
        * static_cast<int64_t> (super_block_.main_block_size_);

      if (static_cast<int64_t> (used_main_bytes * 1.1) < used_ext_bytes)
      {
        used_bytes = used_ext_bytes;
      }
      else                      // just consider used main bytes
      {
        used_bytes = used_main_bytes;
      }

      total_bytes = static_cast<int64_t> (super_block_.main_block_count_)
        * static_cast<int64_t> (super_block_.main_block_size_)
        + static_cast<int64_t> (super_block_.extend_block_count_)
        * static_cast<int64_t> (super_block_.extend_block_size_);
      return TFS_SUCCESS;
    }

    int BlockFileManager::load_super_blk(const FileSystemParameter& fs_param)
    {
      bool fs_init_status = true;

      TBSYS_LOG(TFDEBUG, "read super block. mount name: %s, offset: %d\n", fs_param.mount_name_.c_str(),
                fs_param.super_block_reserve_offset_);
      super_block_impl_ = new SuperBlockImpl(fs_param.mount_name_, fs_param.super_block_reserve_offset_);
      int ret = super_block_impl_->read_super_blk(super_block_);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, "read super block error. ret: %d, desc: %s\n", ret, strerror(errno));
        return ret;
      }

      // return false
      if (!super_block_impl_->check_status(DEV_TAG, super_block_))
      {
        fs_init_status = false;
      }

      if (!fs_init_status)
      {
        TBSYS_LOG(TFERROR, "file system not inited. please format it first.\n");
        return EXIT_FS_NOTINIT_ERROR;
      }

      if (fs_param.mount_name_.compare(super_block_.mount_point_) != 0)
      {
        TBSYS_LOG(TFWARN, "mount point conflict, rewrite mount point. former: %s, now: %s.\n",
            super_block_.mount_point_, fs_param.mount_name_.c_str());
        strncpy(super_block_.mount_point_, fs_param.mount_name_.c_str(), MAX_DEV_NAME_LEN - 1);
        TBSYS_LOG(TFINFO, "super block mount point: %s.", super_block_.mount_point_);
        super_block_impl_->write_super_blk(super_block_);
        super_block_impl_->flush_file();
      }

      unsigned item_count = super_block_.main_block_count_ + super_block_.extend_block_count_ + 1;
      TBSYS_LOG(TFDEBUG, "file system bitmap size: %u\n", item_count);
      normal_bit_map_ = new BitMap(item_count);
      error_bit_map_ = new BitMap(item_count);

      // load bitmap
      bit_map_size_ = normal_bit_map_->get_slot_count();
      char* tmp_bit_map_buf = new char[4 * bit_map_size_];
      memset(tmp_bit_map_buf, 0, 4 * bit_map_size_);

      ret = super_block_impl_->read_bit_map(tmp_bit_map_buf, 4 * bit_map_size_);
      if (TFS_SUCCESS != ret)
      {
        tbsys::gDeleteA(tmp_bit_map_buf);
        return ret;
      }

      // compare two bitmap
      if (memcmp(tmp_bit_map_buf, tmp_bit_map_buf + bit_map_size_, bit_map_size_) != 0)
      {
        tbsys::gDeleteA(tmp_bit_map_buf);
        TBSYS_LOG(TFERROR, "normal bitmap conflict");
        return EXIT_BITMAP_CONFLICT_ERROR;
      }

      if (memcmp(tmp_bit_map_buf + 2 * bit_map_size_, tmp_bit_map_buf + 3 * bit_map_size_, bit_map_size_) != 0)
      {
        tbsys::gDeleteA(tmp_bit_map_buf);
        TBSYS_LOG(TFERROR, "error bitmap conflict");
        return EXIT_BITMAP_CONFLICT_ERROR;
      }

      //load bitmap
      normal_bit_map_->copy(bit_map_size_, tmp_bit_map_buf);
      error_bit_map_->copy(bit_map_size_, tmp_bit_map_buf + 2 * bit_map_size_);
      TBSYS_LOG(TFDEBUG, "bitmap used count: %u, error: %u", normal_bit_map_->get_set_count(),
                error_bit_map_->get_set_count());
      tbsys::gDeleteA(tmp_bit_map_buf);

      return TFS_SUCCESS;
    }

    int BlockFileManager::load_block_file()
    {
      // construct logic block and corresponding physicblock list according to bitmap
      PhysicalBlockMapIter pit;
      bool conflict_flag = false;
      PhysicalBlock* t_physical_block = NULL;
      PhysicalBlock* ext_physical_block = NULL;
      LogicBlock* t_logic_block = NULL;
      int ret = TFS_SUCCESS;
      // traverse bitmap
      for (uint32_t pos = 1; pos <= static_cast<uint32_t> (super_block_.main_block_count_); ++pos)
      {
        // init every loop
        t_physical_block = NULL;
        t_logic_block = NULL;
        ext_physical_block = NULL;
        
        // 1. test bitmap find
        if (normal_bit_map_->test(pos))
        {
          // 2. construct new physic block
          t_physical_block = new PhysicalBlock(pos, super_block_.mount_point_, BLOCK_RESERVER_LENGTH+super_block_.main_block_size_,
                                               C_MAIN_BLOCK);
          // 3. read physical block's head,check prefix
          ret = t_physical_block->load_block_prefix();
          if (TFS_SUCCESS != ret)
          {
            tbsys::gDelete(t_physical_block);
            TBSYS_LOG(TFERROR, "init physical block fail. fatal error! pos: %d, file: %s", pos, super_block_.mount_point_);
            break;
          }

          BlockPrefix block_prefix;
          t_physical_block->get_block_prefix(block_prefix);
          // not inited, error
          if (0 == block_prefix.fileid && 0 == block_prefix.chunkid)
          {
            tbsys::gDelete(t_physical_block);
            TBSYS_LOG(TFERROR, "load pysical block prefix illegal! physical pos: %u.",pos);
            normal_bit_map_->reset(pos);

            // write or flush fail, just log, not exit. not dirty global ret value
            int tmp_ret;
            if ((tmp_ret = super_block_impl_->write_bit_map(normal_bit_map_, error_bit_map_)) != TFS_SUCCESS)
              TBSYS_LOG(TFERROR, "write bit map fail %d", tmp_ret);
            if ((tmp_ret = super_block_impl_->flush_file()) != TFS_SUCCESS)
              TBSYS_LOG(TFERROR, "flush super block fail %d", tmp_ret);

            continue;
          }

          // 4. construct logic block, add physic block
          t_logic_block = new LogicBlock(block_prefix.fileid,block_prefix.chunkid);
          t_logic_block->add_physic_block(t_physical_block);

          // record physical block id
          uint64_t fileid = block_prefix.fileid;
          uint32_t chunkid = block_prefix.chunkid;
          TBSYS_LOG(
            TFDEBUG,
            "load logic block. logic blockid: <%Lu,%u>, physic prev blockid: %u, physic next blockid: %u, physical blockid: %u.",
            fileid,chunkid,block_prefix.prev_physic_blockid_,
            block_prefix.next_physic_blockid_, pos);

          // 5. make sure this physical block hasn't been loaded
          pit = physcial_blocks_.find(pos);
          if (pit != physcial_blocks_.end())
          {
            tbsys::gDelete(t_physical_block);
            tbsys::gDelete(t_logic_block);
            ret = EXIT_PHYSIC_UNEXPECT_FOUND_ERROR;
            TBSYS_LOG(TFERROR, "logic blockid<%Lu,%u>, physical blockid: %u is repetitive. fatal error! ret: %d",
                      fileid,chunkid, pos, EXIT_PHYSIC_UNEXPECT_FOUND_ERROR);
            break;
          }

          // 6. find if it is a existed logic block.
          // if exist, delete the exist one(this scene is appear in the compact process: server down after set clean),
          if(find_logic_block(fileid,chunkid,C_MAIN_BLOCK))
          {
            TBSYS_LOG(TFDEBUG, "logic blockid<%Lu,%u> already exist", fileid,chunkid);
            if (find_logic_block(fileid,chunkid,C_COMPACT_BLOCK) )
            {
              TBSYS_LOG(TFDEBUG, "logic blockid: <%Lu,%u> already exist in compact block map", fileid,chunkid);
              ret = del_block(fileid,chunkid, C_COMPACT_BLOCK);
              if (TFS_SUCCESS != ret)
              {
                tbsys::gDelete(t_physical_block);
                tbsys::gDelete(t_logic_block);
                break;
              }
            }
            insert_logic_block_map(compact_logic_blocks_, t_logic_block);
          }
          else                  // not exist, insert
          {
            insert_logic_block_map(logic_blocks_,t_logic_block);
          }

          // 7. insert physic block to physic map
          physcial_blocks_.insert(PhysicalBlockMap::value_type(pos, t_physical_block));

          uint32_t ext_pos = pos;
          // 8. add extend physical block
          while (0 != block_prefix.next_physic_blockid_)
          {
            ext_physical_block = NULL;
            uint32_t prev_block_id = ext_pos;
            ext_pos = block_prefix.next_physic_blockid_;

            //bitmap is not set
            //program exit
            if (!normal_bit_map_->test(ext_pos))
            {
              TBSYS_LOG(TFERROR, "ext next physicblock and bitmap conflicted! logic blockid: <%Lu,%u>, physical blockid: %u.",
                        fileid,chunkid, ext_pos);
              ret = EXIT_BLOCKID_CONFLICT_ERROR;
              break;
            }

            ext_physical_block = new PhysicalBlock(ext_pos, super_block_.mount_point_,
                                                   BLOCK_RESERVER_LENGTH+super_block_.extend_block_size_, C_EXT_BLOCK);
            ret = ext_physical_block->load_block_prefix();
            if (TFS_SUCCESS != ret)
            {
              tbsys::gDelete(ext_physical_block);
              TBSYS_LOG(TFERROR, "init physical block fail. fatal error! pos: %u, mount point: %s", pos,
                        super_block_.mount_point_);
              break;
            }

            memset(&block_prefix, 0, sizeof(BlockPrefix));
            ext_physical_block->get_block_prefix(block_prefix);
            if (prev_block_id != block_prefix.prev_physic_blockid_)
            {
              TBSYS_LOG(TFERROR, "read prev blockid conflict! prev blockid: %u. block prefix's physic prev blockid: %u",
                        prev_block_id, block_prefix.prev_physic_blockid_);
              //release
              normal_bit_map_->reset(pos);


            // write or flush fail, just log, not exit. not dirty global ret value
              int tmp_ret;
              if ((tmp_ret = super_block_impl_->write_bit_map(normal_bit_map_, error_bit_map_)) != TFS_SUCCESS)
                TBSYS_LOG(TFERROR, "write bit map fail %d", tmp_ret);
              if ((tmp_ret = super_block_impl_->flush_file()) != TFS_SUCCESS)
                TBSYS_LOG(TFERROR, "flush super block fail %d", tmp_ret);
              conflict_flag = true;
              tbsys::gDelete(ext_physical_block);
              break;
            }

            TBSYS_LOG(
              TFDEBUG,
              "read blockprefix! ext physical blockid pos: %u. prev blockid: %u. blockprefix's physic prev blockid: %u, next physic blockid: %u, logic blockid :<%Lu,%u>",
              ext_pos, prev_block_id, block_prefix.prev_physic_blockid_, block_prefix.next_physic_blockid_,
              block_prefix.fileid,block_prefix.chunkid);
            pit = physcial_blocks_.find(ext_pos);
            if (pit != physcial_blocks_.end())
            {
              tbsys::gDelete(ext_physical_block);
              ret = EXIT_PHYSIC_UNEXPECT_FOUND_ERROR;
              TBSYS_LOG(TFERROR, "logic blockid: <%Lu,%u>, physical blockid: %u is repetitive. fatal error!", 
              fileid,chunkid,ext_pos);
              tbsys::gDelete(ext_physical_block);
              break;
            }

            t_logic_block->add_physic_block(ext_physical_block);
            physcial_blocks_.insert(PhysicalBlockMap::value_type(ext_pos, ext_physical_block));
          }

          if (TFS_SUCCESS != ret)
          {
            break;
          }

          if (conflict_flag)
          {
            TBSYS_LOG(TFERROR, "delete load conflict block. logic blockid: <%Lu,%u>", fileid,chunkid);
            del_block(fileid,chunkid, C_CONFUSE_BLOCK);
            continue;
          }
          // 9. load logic block...
        }
        conflict_flag = false;
      }

      return ret;
    }

    int BlockFileManager::find_avail_block(uint32_t& ext_physical_block_id, const BlockType block_type)
    {
      int32_t i = 1;
      int32_t size = super_block_.main_block_count_;
      if (C_EXT_BLOCK == block_type)
      {
        i = super_block_.main_block_count_ + 1;
        size += super_block_.extend_block_count_;
      }

      bool hit_block = false;
      for (; i <= size; ++i)
      {
        //find first avaiable block
        if (!normal_bit_map_->test(i))
        {
          //test error bitmap
          if (error_bit_map_->test(i)) //skip error block
          {
            continue;
          }

          hit_block = true;
          break;
        }
      }

      // find nothing
      if (!hit_block)
      {
        TBSYS_LOG(TFERROR, "block is exhausted! blocktype: %d\n", block_type);
        return EXIT_BLOCK_EXHAUST_ERROR;
      }

      TBSYS_LOG(TFDEBUG, "find avail blockid: %u\n", i);
      ext_physical_block_id = i;
      return TFS_SUCCESS;
    }

    int BlockFileManager::init_super_blk_param(const FileSystemParameter& fs_param)
    {
      memset((void *) &super_block_, 0, sizeof(SuperBlock));
      memcpy(super_block_.mount_tag_, DEV_TAG, sizeof(super_block_.mount_tag_));
      super_block_.time_ = time(NULL);
      strncpy(super_block_.mount_point_, fs_param.mount_name_.c_str(), MAX_DEV_NAME_LEN - 1);
      TBSYS_LOG(TFDEBUG, "super block mount point: %s.", super_block_.mount_point_);
      int32_t scale = 1024;
      super_block_.mount_point_use_space_ = fs_param.max_mount_size_ * scale;
      super_block_.base_fs_type_ = static_cast<BaseFsType> (fs_param.base_fs_type_);

      if (EXT4 != super_block_.base_fs_type_ && EXT3_FULL != super_block_.base_fs_type_ && EXT3_FTRUN
          != super_block_.base_fs_type_)
      {
        TBSYS_LOG(TFERROR, "base fs type is not supported. base fs type: %d", super_block_.base_fs_type_);
        return TFS_ERROR;
      }

      super_block_.superblock_reserve_offset_ = fs_param.super_block_reserve_offset_;
      // leave sizeof(int)
      super_block_.bitmap_start_offset_ = super_block_.superblock_reserve_offset_ + 2 * sizeof(SuperBlock)
          + sizeof(int32_t);
      super_block_.block_type_ratio_ = fs_param.block_type_ratio_;

      int64_t avail_data_space = static_cast<int64_t> (super_block_.mount_point_use_space_);

      super_block_.main_block_size_ = fs_param.main_block_size_;
      super_block_.extend_block_size_ = fs_param.extend_block_size_;

      int32_t main_block_count = 0, extend_block_count = 0;
      calc_block_count(avail_data_space, main_block_count, extend_block_count);
      super_block_.main_block_count_ = main_block_count;
      super_block_.extend_block_count_ = extend_block_count;

      super_block_.used_block_count_ = 0;
      super_block_.used_extend_block_count_ = 0;
      super_block_.version_ = fs_param.file_system_version_;

      super_block_.display();
      return TFS_SUCCESS;
    }

    void BlockFileManager::calc_block_count(const int64_t avail_data_space, int32_t& main_block_count,
        int32_t& ext_block_count)
    {
#if EXTEND_SUPPORT
      ext_block_count = static_cast<int32_t> (static_cast<float> (avail_data_space) / (super_block_.block_type_ratio_
          * static_cast<float> (super_block_.main_block_size_) + static_cast<float> (super_block_.extend_block_size_)));
      main_block_count = static_cast<int32_t> (static_cast<float> (ext_block_count) * super_block_.block_type_ratio_);
#else
    main_block_count = static_cast<int32_t> (static_cast<float> (avail_data_space) / static_cast<float> (super_block_.main_block_size_));
    ext_block_count = 0;
    super_block_.block_type_ratio_ = -1;
    super_block_.extend_block_size_ = -1;
#endif
      TBSYS_LOG(TFDEBUG,
          "cal block count. avail data space: %" PRI64_PREFIX "d, main block count: %d, ext block count: %d",
          avail_data_space, main_block_count, ext_block_count);
    }

    int BlockFileManager::create_fs_dir()
    {
      //super_block_.display();
      int ret = mkdir(super_block_.mount_point_, DIR_MODE);
      if (ret && errno != EEXIST)
      {
        TBSYS_LOG(TFERROR, "make extend dir error. dir: %s, ret: %d, error: %d, error desc: %s",
            super_block_.mount_point_, ret, errno, strerror(errno));
        return TFS_ERROR;
      }

      // extend block directory
      std::string extend_dir = super_block_.mount_point_;
      extend_dir += EXTENDBLOCK_DIR_PREFIX;
      ret = mkdir(extend_dir.c_str(), DIR_MODE);
      if (ret)
      {
        TBSYS_LOG(TFERROR, "make extend dir:%s error. ret: %d, error: %s", extend_dir.c_str(), ret, strerror(errno));
        return TFS_ERROR;
      }

      // index file directory
      std::string index_dir = super_block_.mount_point_;
      index_dir += INDEX_DIR_PREFIX;
      ret = mkdir(index_dir.c_str(), DIR_MODE);
      if (ret)
      {
        TBSYS_LOG(TFERROR, "make index dir error. ret: %d, error: %d", ret, errno);
        return TFS_ERROR;
      }

      return TFS_SUCCESS;
    }

    uint32_t BlockFileManager::calc_bitmap_count()
    {
      uint32_t item_count = super_block_.main_block_count_ + super_block_.extend_block_count_ + 1;
      BitMap tmp_bit_map(item_count);
      uint32_t slot_count = tmp_bit_map.get_slot_count();
      TBSYS_LOG(TFDEBUG, "cal bitmap count. item count: %u, slot count: %u", item_count, slot_count);
      return slot_count;
    }

    int BlockFileManager::create_fs_super_blk()
    {
      uint32_t bit_map_size = calc_bitmap_count();
      int super_block_file_size = 2 * sizeof(SuperBlock) + sizeof(int32_t) + 4 * bit_map_size;

      char* tmp_buffer = new char[super_block_file_size];
      memcpy(tmp_buffer, &super_block_, sizeof(SuperBlock));
      memcpy(tmp_buffer + sizeof(SuperBlock), &super_block_, sizeof(SuperBlock));
      memset(tmp_buffer + 2 * sizeof(SuperBlock), 0, 4 * bit_map_size + sizeof(int));

      std::string super_block_file = super_block_.mount_point_;
      super_block_file += SUPERBLOCK_NAME;
      FileOperation* super_file_op = new FileOperation(super_block_file, O_RDWR | O_CREAT);
      int ret = super_file_op->pwrite_file(tmp_buffer, super_block_file_size, 0);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, "write super block file error. ret: %d.", ret);
      }

      tbsys::gDelete(super_file_op);
      tbsys::gDeleteA(tmp_buffer);

      return ret;
    }

    int BlockFileManager::create_block(const BlockType block_type)
    {
      int32_t prefix_size = sizeof(BlockPrefix);
      char* block_prefix = new char[prefix_size];
      memset(block_prefix, 0, prefix_size);
      FileFormater* file_formater = NULL;

      if (EXT4 == super_block_.base_fs_type_)
      {
        file_formater = new Ext4FileFormater();
      }
      else if (EXT3_FULL == super_block_.base_fs_type_)
      {
        file_formater = new Ext3FullFileFormater();
      }
      else if (EXT3_FTRUN == super_block_.base_fs_type_)
      {
        file_formater = new Ext3SimpleFileFormater();
      }
      else
      {
        TBSYS_LOG(TFERROR, "base fs type is not supported. base fs type: %d", super_block_.base_fs_type_);
        tbsys::gDeleteA(block_prefix);
        return TFS_ERROR;
      }

      int32_t block_count = 0;
      int32_t block_size = 0;
      if (C_MAIN_BLOCK == block_type)
      {
        block_count = super_block_.main_block_count_;
        block_size = super_block_.main_block_size_;
      }
      else if (C_EXT_BLOCK == block_type)
      {
        block_count = super_block_.extend_block_count_;
        block_size = super_block_.extend_block_size_;
      }
      else
      {
        tbsys::gDeleteA(block_prefix);
        tbsys::gDelete(file_formater);
        return TFS_ERROR;
      }

      for (int32_t i = 1; i <= block_count; ++i)
      {
        std::string block_file;
        std::stringstream tmp_stream;
        if (C_MAIN_BLOCK == block_type)
        {
          tmp_stream << super_block_.mount_point_ << MAINBLOCK_DIR_PREFIX << i;
        }
        else
        {
          tmp_stream << super_block_.mount_point_ << EXTENDBLOCK_DIR_PREFIX << (i + super_block_.main_block_count_);
        }
        tmp_stream >> block_file;
        FileOperation* file_op = new FileOperation(block_file, O_RDWR | O_CREAT);
        int ret = file_op->open_file();
        if (ret < 0)
        {
          TBSYS_LOG(TFERROR, "allocate space error. ret: %d, error: %d, error desc: %s\n", ret, errno, strerror(errno));
          tbsys::gDelete(file_op);
          tbsys::gDeleteA(block_prefix);
          tbsys::gDelete(file_formater);
          tbsys::gDelete(file_op);
          tbsys::gDeleteA(block_prefix);
          tbsys::gDelete(file_formater);
          return ret;
        }

        ret = file_formater->block_file_format(file_op->get_fd(), BLOCK_RESERVER_LENGTH+block_size);
        if (TFS_SUCCESS != ret)
        {
          tbsys::gDelete(file_op);
          tbsys::gDeleteA(block_prefix);
          tbsys::gDelete(file_formater);
          TBSYS_LOG(TFERROR, "allocate space error. ret: %d, error: %d, error desc: %s\n", ret, errno, strerror(errno));
          tbsys::gDelete(file_op);
          tbsys::gDeleteA(block_prefix);
          tbsys::gDelete(file_formater);
          return ret;
        }
        ret = file_op->pwrite_file(block_prefix, prefix_size, 0);
        if (TFS_SUCCESS != ret)
        {
          tbsys::gDelete(file_op);
          tbsys::gDeleteA(block_prefix);
          tbsys::gDelete(file_formater);
          TBSYS_LOG(TFERROR, "write block file error. physcial blockid: %d, block type: %d, ret: %d.", i, block_type,
              ret);
          tbsys::gDelete(file_op);
          tbsys::gDeleteA(block_prefix);
          tbsys::gDelete(file_formater);
          return ret;
        }
        tbsys::gDelete(file_op);
      }

      tbsys::gDeleteA(block_prefix);
      tbsys::gDelete(file_formater);
      return TFS_SUCCESS;
    }

    LogicBlock* BlockFileManager::find_logic_block(uint64_t fileid, uint32_t chunkid, const BlockType block_type)
    {
      LogicBlockMap* selected_logic_blocks;
      if (C_COMPACT_BLOCK == block_type)
      {
        selected_logic_blocks = &compact_logic_blocks_;
      }
      else if (C_MAIN_BLOCK == block_type)
      {
        selected_logic_blocks = &logic_blocks_;
      }
      else
      {
        return NULL;
      }
      return find_logic_block_map(*selected_logic_blocks,fileid,chunkid);
    }

    int BlockFileManager::erase_logic_block(uint64_t fileid,uint32_t chunkid, const BlockType block_type)
    {
      LogicBlockMap* selected_logic_blocks;
      if (C_COMPACT_BLOCK == block_type)
      {
        selected_logic_blocks = &compact_logic_blocks_;
      }
      else // base on current types, main Block
      {
        selected_logic_blocks = &logic_blocks_;
      }
      return delete_logic_block_map(*selected_logic_blocks,fileid,chunkid);
    }

    void BlockFileManager::rollback_superblock(const uint32_t physical_block_id, const bool modify_flag)
    {
      if (normal_bit_map_->test(physical_block_id))
      {
        normal_bit_map_->reset(physical_block_id);
        int ret = super_block_impl_->write_bit_map(normal_bit_map_, error_bit_map_);
        if (TFS_SUCCESS != ret)
          TBSYS_LOG(TFERROR, "write bit map fail. ret: %d, physical blockid: %u", ret, physical_block_id);

        if (modify_flag)
        {
          --super_block_.used_extend_block_count_;
          ret = super_block_impl_->write_super_blk(super_block_);
          if (TFS_SUCCESS != ret)
            TBSYS_LOG(TFERROR, "write super block fail. ret: %d", ret);
        }

        ret = super_block_impl_->flush_file();
        if (TFS_SUCCESS != ret)
          TBSYS_LOG(TFERROR, "flush super block fail. ret: %d", ret);
      }
      return;
    }
    
    LogicBlock* BlockFileManager::find_logic_block_map(LogicBlockMap& logicmap,uint64_t fileid,uint32_t chunkid)
    {
        LogicBlockMapIter FileIter;
        LogicChunkIter    ChunkIter;
        if( ((FileIter = logicmap.find(fileid)) == logicmap.end()) ||
           (ChunkIter = FileIter->second.find(chunkid)) == FileIter->second.end() )
        {
            return NULL;
        }
        return ChunkIter->second; 
    }
    int BlockFileManager::insert_logic_block_map(LogicBlockMap& logicmap,LogicBlock* block,uint64_t fileid,uint32_t chunkid)
    {
        if(block == NULL||fileid != block->file_id()||chunkid != block->chunk_id())
            return TFS_ERROR;
        logicmap[fileid][chunkid] = block;
        return TFS_SUCCESS;
    }
    int BlockFileManager::insert_logic_block_map(LogicBlockMap& logicmap,LogicBlock* block)
    {   
        if(block == NULL)
            return TFS_ERROR;
        logicmap[block->file_id()][block->chunk_id()] = block;
        return TFS_SUCCESS;
    }
    int BlockFileManager::delete_logic_block_map(LogicBlockMap& logicmap,uint64_t fileid,uint32_t chunkid)
    {
        LogicBlockMapIter FileIter;
        LogicChunkIter    ChunkIter;
        if( ((FileIter = logicmap.find(fileid)) == logicmap.end()) ||
           (ChunkIter = FileIter->second.find(chunkid)) == FileIter->second.end() )
        {
            return EXIT_NO_LOGICBLOCK_ERROR;
        }
        FileIter->second.erase(chunkid);
        if(FileIter->second.empty())
            logicmap.erase(fileid);
        return TFS_SUCCESS;  
    }
    uint64_t BlockFileManager::get_logic_block_size_map(const BlockType block_type)
    {
        uint64_t size = 0;
        LogicBlockMap& selected_logic_blocks = logic_blocks_;
        if (C_COMPACT_BLOCK == block_type)
        {
            selected_logic_blocks = compact_logic_blocks_;
        }
        else // base on current types, main Block
        {
            selected_logic_blocks = logic_blocks_;
        }
        for(LogicBlockMapIter FileIter = selected_logic_blocks.begin();
             FileIter != selected_logic_blocks.end();++FileIter)
                 size += FileIter->second.size();
        return size;
    }
  }
}
