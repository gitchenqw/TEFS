
#include <inttypes.h>
#include "data_management.h"
#include "blockfile_manager.h"
#include "dataserver_define.h"
#include "common/tbsys.h"
#include "visit_stat.h"

namespace tfs
{
  namespace dataserver
  {

    using namespace common;
    using namespace std;

    DataManagement::DataManagement() :
      file_number_(0), last_gc_data_file_time_(0)
    {
    }

    DataManagement::~DataManagement()
    {
    }

    void DataManagement::set_file_number(const uint64_t file_number)
    {
      file_number_ = file_number;
      TBSYS_LOG(TFINFO, "set file number. file number: %" PRI64_PREFIX "u\n", file_number_);
    }

    int DataManagement::init_block_files(const FileSystemParameter& fs_param)
    {
      int64_t time_start = tbsys::CTimeUtil::getTime();
      TBSYS_LOG(TFDEBUG, "block file load blocks begin. start time: %" PRI64_PREFIX "d\n", time_start);
      // just start up
      int ret = BlockFileManager::get_instance()->bootstrap(fs_param);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, "blockfile manager boot fail! ret: %d\n", ret);
        return ret;
      }
      int64_t time_end = tbsys::CTimeUtil::getTime();
      TBSYS_LOG(TFDEBUG, "block file load blocks end. end time: %" PRI64_PREFIX "d. cost time: %" PRI64_PREFIX "d.",
          time_end, time_end - time_start);
      return TFS_SUCCESS;
    }

    void DataManagement::get_ds_filesystem_info(int32_t& block_count, int64_t& use_capacity, int64_t& total_capacity)
    {
      BlockFileManager::get_instance()->query_approx_block_count(block_count);
      BlockFileManager::get_instance()->query_space(use_capacity, total_capacity);
      return;
    }

    int DataManagement::get_all_logic_block(std::list<LogicBlock*>& logic_block_list)
    {
      return BlockFileManager::get_instance()->get_all_logic_block(logic_block_list);
    }

    int64_t DataManagement::get_all_logic_block_size()
    {
      return BlockFileManager::get_instance()->get_all_logic_block_size();
    }

/*    int DataManagement::write_data(const WriteDataInfo& write_info,
    const char* data_buffer) //write data to data_file
    {
      // write data to DataFile first
      data_file_mutex_.lock();
      DataFileMapIter bit = data_file_map_.find(write_info.file_number_);
      DataFile* datafile = NULL;
      if (bit != data_file_map_.end())
      {
        datafile = bit->second;
      }
      else                      // not found
      {
        // control datafile size
        if (data_file_map_.size() >= static_cast<uint32_t> (SYSPARAM_DATASERVER.max_datafile_nums_))
        {
          TBSYS_LOG(ERROR, "blockid: %u, datafile nums: %u is large than default.", write_info.block_id_,
              data_file_map_.size());
          data_file_mutex_.unlock();
          return EXIT_DATAFILE_OVERLOAD;
        }

        datafile = new DataFile(write_info.file_number_);
        data_file_map_.insert(DataFileMap::value_type(write_info.file_number_, datafile));
      }

      if (NULL == datafile)
      {
        TBSYS_LOG(ERROR, "datafile is null. blockid: %u, fileid: %" PRI64_PREFIX "u, filenumber: %" PRI64_PREFIX "u",
            write_info.block_id_, write_info.file_id_, write_info.file_number_);
        data_file_mutex_.unlock();
        return EXIT_DATA_FILE_ERROR;
      }
      datafile->set_last_update();
      data_file_mutex_.unlock();

      // write to datafile
      int32_t write_len = datafile->set_data(data_buffer, write_info.length_, write_info.offset_);
      if (write_len != write_info.length_)
      {
        TBSYS_LOG(
            ERROR,
            "Datafile write error. blockid: %u, fileid: %" PRI64_PREFIX "u, filenumber: %" PRI64_PREFIX "u, req writelen: %d, actual writelen: %d",
            write_info.block_id_, write_info.file_id_, write_info.file_number_, write_info.length_, write_len);
        // clean dirty data
        erase_data_file(write_info.file_number_);
        return EXIT_DATA_FILE_ERROR;
      }

      return TFS_SUCCESS;
    }
    
    int DataManagement::close_write_file(const CloseFileInfo& closefileInfo, int32_t& write_file_size)
    {
      uint32_t block_id = closefileInfo.block_id_;
      uint64_t file_id = closefileInfo.file_id_;
      uint64_t file_number = closefileInfo.file_number_;
      uint32_t crc = closefileInfo.crc_;
      TBSYS_LOG(DEBUG,
          "close write file, blockid: %u, fileid: %" PRI64_PREFIX "u, filenumber: %" PRI64_PREFIX "u, crc: %u",
          block_id, file_id, file_number, crc);

      //find datafile
      DataFile* datafile = NULL;
      data_file_mutex_.lock();
      DataFileMapIter bit = data_file_map_.find(file_number);
      if (bit != data_file_map_.end())
      {
        datafile = bit->second;
      }

      //lease expire
      if (NULL == datafile)
      {
        TBSYS_LOG(ERROR, "Datafile is null. blockid: %u, fileid: %" PRI64_PREFIX "u, filenumber: %" PRI64_PREFIX "u",
            block_id, file_id, file_number);
        data_file_mutex_.unlock();
        return EXIT_DATAFILE_EXPIRE_ERROR;
      }
      datafile->set_last_update();
      datafile->add_ref();
      data_file_mutex_.unlock();

      //compare crc
      uint32_t datafile_crc = datafile->get_crc();
      if (crc != datafile_crc)
      {
        TBSYS_LOG(
            ERROR,
            "Datafile crc error. blockid: %u, fileid: %" PRI64_PREFIX "u, filenumber: %" PRI64_PREFIX "u, local crc: %u, msg crc: %u",
            block_id, file_id, file_number, datafile_crc, crc);
        datafile->sub_ref();
        erase_data_file(file_number);
        return EXIT_DATA_FILE_ERROR;
      }

      write_file_size = datafile->get_length();
      //find block
      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(block_id);
      if (NULL == logic_block)
      {
        datafile->sub_ref();
        erase_data_file(file_number);
        TBSYS_LOG(ERROR, "blockid: %u is not exist.", block_id);
        return EXIT_NO_LOGICBLOCK_ERROR;
      }

      TIMER_START();
      int ret = logic_block->close_write_file(file_id, datafile, datafile_crc);
      if (TFS_SUCCESS != ret)
      {
        datafile->sub_ref();
        erase_data_file(file_number);
        return ret;
      }

      TIMER_END();
      if (TIMER_DURATION() > SYSPARAM_DATASERVER.max_io_warn_time_)
      {
        TBSYS_LOG(WARN, "write file cost time: blockid: %u, fileid: %" PRI64_PREFIX "u, cost time: %" PRI64_PREFIX "d",
            block_id, file_id, TIMER_DURATION());
      }

      // success, gc datafile
      // close tmp file, release opened file handle
      // datafile , bit->second point to same thing, once delete
      // bit->second, datafile will be obseleted immediately.
      datafile->sub_ref();
      erase_data_file(file_number);
      return TFS_SUCCESS;
    }
    */
    int DataManagement::erase_data_file(const uint64_t file_number)
    {
      data_file_mutex_.lock();
      DataFileMapIter bit = data_file_map_.find(file_number);
      if (bit != data_file_map_.end() && NULL != bit->second)
      {
        tbsys::gDelete(bit->second);
        data_file_map_.erase(bit);
      }
      data_file_mutex_.unlock();
      return TFS_SUCCESS;
    }

    int DataManagement::read_raw_data(uint64_t fileid, uint32_t chunkid,const int32_t read_offset, 
                                        int32_t& real_read_len,char* tmp_data_buffer)
    {
      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(fileid,chunkid);
      if (NULL == logic_block)
      {
        TBSYS_LOG(TFERROR, "block not exist, blockid: <%" PRIu64 ",%u>", fileid,chunkid);
        return EXIT_NO_LOGICBLOCK_ERROR;
      }

      int ret = logic_block->read_raw_data(tmp_data_buffer, real_read_len, read_offset);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, "blockid: <%" PRIu64 ",%u> read data batch error, offset: %d, rlen: %d, ret: %d", fileid,chunkid,read_offset,
            real_read_len, ret);
        return ret;
      }

      return TFS_SUCCESS;
    }

    int DataManagement::batch_new_block(const VINT64* new_blocks)//fileid+ chunkid
    {
      int ret = TFS_SUCCESS;
      if (NULL != new_blocks)
      {
        for (uint32_t i = 0; i < new_blocks->size(); i += 2)
        {
          TBSYS_LOG(TFINFO, "new block: <%" PRIu64 ",%u>\n", new_blocks->at(i),new_blocks->at(i+1));
          uint32_t physic_block_id = 0;
          ret = BlockFileManager::get_instance()->new_block(new_blocks->at(i),new_blocks->at(i+1),
                                                            physic_block_id);
          if (TFS_SUCCESS != ret)
          {
            TBSYS_LOG(TFERROR, "new block fail, blockid: <%" PRIu64 ",%u> ret: %d",
                        new_blocks->at(i),new_blocks->at(i+1), ret);
            return ret;
          }
          else
          {
            TBSYS_LOG(TFINFO, "new block successful, blockid: %s, phyical blockid:<%" PRIu64 ", %u>",
                        new_blocks->at(i),new_blocks->at(i+1),physic_block_id);
          }
        }
      }

      return ret;
    }

    int DataManagement::batch_remove_block(const VINT64* remove_blocks)//fileid+ chunkid
    {
      int ret = TFS_SUCCESS;
      if (NULL != remove_blocks)
      {
        for (uint32_t i = 0; i < remove_blocks->size(); i += 2)
        {
          TBSYS_LOG(TFINFO, "remove block: <%" PRIu64 ",%u>", remove_blocks->at(i), remove_blocks->at(i+1));
          ret = BlockFileManager::get_instance()->del_block(remove_blocks->at(i),remove_blocks->at(i+1));
          if (TFS_SUCCESS != ret)
          {
            TBSYS_LOG(TFERROR, "remove block error, blockid: <%" PRIu64 ",%u>, ret: %d", remove_blocks->at(i), 
                                remove_blocks->at(i+1),ret);
            return ret;
          }
          else
          {
            TBSYS_LOG(TFINFO, "remove block successful, blockid: <%" PRIu64 ",%u>", remove_blocks->at(i), 
                                remove_blocks->at(i+1));
          }
        }
      }

      return ret;
    }

    int DataManagement::query_bit_map(const int32_t query_type, char** tmp_data_buffer, int32_t& bit_map_len,
        int32_t& set_count)
    {
      // the caller should release the tmp_data_buffer memory
      if (NORMAL_BIT_MAP == query_type)
      {
        BlockFileManager::get_instance()->query_bit_map(tmp_data_buffer, bit_map_len, set_count, C_ALLOCATE_BLOCK);
      }
      else
      {
        BlockFileManager::get_instance()->query_bit_map(tmp_data_buffer, bit_map_len, set_count, C_ERROR_BLOCK);
      }

      return TFS_SUCCESS;
    }
    
    int DataManagement::query_block_status(const int32_t query_type, common::VSTRING& block_ids, 
                    std::map<std::string, std::vector<uint32_t> >& logic_2_physic_blocks)
    {
      std::list<LogicBlock*> logic_blocks;
      std::list<LogicBlock*>::iterator lit;

      BlockFileManager::get_instance()->get_logic_block_ids(block_ids);
      BlockFileManager::get_instance()->get_all_logic_block(logic_blocks);

      if (query_type & LB_PAIRS) // logick block ==> physic block list
      {
        std::list<PhysicalBlock*>* phy_blocks;
        std::list<PhysicalBlock*>::iterator pit;

        for (lit = logic_blocks.begin(); lit != logic_blocks.end(); ++lit)
        {
          if (*lit)
          {
            TBSYS_LOG(TFDEBUG, "query block status, query type: %d, blockid: <%" PRIu64 ",%u>\n", query_type,
                (*lit)->file_id(),(*lit)->chunk_id());
            phy_blocks = (*lit)->get_physic_block_list();
            std::vector < uint32_t > phy_block_ids;
            for (pit = phy_blocks->begin(); pit != phy_blocks->end(); ++pit)
            {
              phy_block_ids.push_back((*pit)->get_physic_block_id());
            }
            char str[32];
            combine_itoa((*lit)->file_id(),(*lit)->chunk_id(),str,10);
            logic_2_physic_blocks.insert(std::map<std::string, std::vector<uint32_t> >::value_type(
                (std::string)str, phy_block_ids)); //only file_id
          }
        }
      }
      return TFS_SUCCESS;
    }
    
    int DataManagement::get_block_info(uint64_t fileid,uint32_t chunkid,int32_t& block_size,int32_t& visit_count)
    {
      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(fileid,chunkid);
      if (NULL == logic_block)
      {
        TBSYS_LOG(TFERROR, "blockid: <%" PRIu64 ",%u> is not exist.", fileid,chunkid);
        return EXIT_NO_LOGICBLOCK_ERROR;
      }
      
      block_size = logic_block->get_logic_block_size();
      visit_count = logic_block->get_visit_count();
      return TFS_SUCCESS;
    }

    int DataManagement::get_visit_sorted_blockids(std::vector<LogicBlock*>& block_ptrs)
    {
      std::list<LogicBlock*> logic_blocks;
      BlockFileManager::get_instance()->get_all_logic_block(logic_blocks);

      for (std::list<LogicBlock*>::iterator lit = logic_blocks.begin(); lit != logic_blocks.end(); ++lit)
      {
        block_ptrs.push_back(*lit);
      }

      sort(block_ptrs.begin(), block_ptrs.end(), visit_count_sort());
      return TFS_SUCCESS;
    }

    int DataManagement::new_single_block(uint64_t fileid,uint32_t chunkid)
    {
      int ret = TFS_SUCCESS;
      // delete if exist
      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(fileid,chunkid);
      if (NULL != logic_block)
      {
        TBSYS_LOG(TFINFO, "block already exist, blockid: <%" PRIu64 ",%u>. first del it", fileid,chunkid);
        ret = BlockFileManager::get_instance()->del_block(fileid,chunkid);
        if (TFS_SUCCESS != ret)
        {
          TBSYS_LOG(TFERROR, "block already exist, blockid:<%" PRIu64 ",%u>. block delete fail. ret: %d", fileid,chunkid,
                    ret);
          return ret;
        }
      }

      uint32_t physic_block_id = 0;
      ret = BlockFileManager::get_instance()->new_block(fileid,chunkid, physic_block_id);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, "block create error, blockid:<%" PRIu64 ",%u>, ret: %d", fileid,chunkid, ret);
        return ret;
      }
      return TFS_SUCCESS;
    }

    int DataManagement::del_single_block(uint64_t fileid,uint32_t chunkid)
    {
      TBSYS_LOG(TFINFO, "remove single block, blockid:<%" PRIu64 ",%u> ", fileid,chunkid);
      return BlockFileManager::get_instance()->del_block(fileid,chunkid);
    }

    int DataManagement::get_block_curr_size(uint64_t fileid,uint32_t chunkid, int32_t& size)
    {
      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(fileid,chunkid);
      if (NULL == logic_block)
      {
        TBSYS_LOG(TFERROR, "blockid:<%" PRIu64 ",%u>  is not exist.", fileid,chunkid);
        return EXIT_NO_LOGICBLOCK_ERROR;
      }

      size = logic_block->get_logic_block_size();
      TBSYS_LOG(TFDEBUG, "blockid:<%" PRIu64 ",%u>  data file size: %d\n", fileid,chunkid, size);
      return TFS_SUCCESS;
    }

    int DataManagement::write_raw_data(uint64_t fileid,uint32_t chunkid, const int32_t data_offset, const int32_t msg_len,
        const char* data_buffer)
    {
      //zero length
      if (0 == msg_len)
      {
        return TFS_SUCCESS;
      }
      int ret = TFS_SUCCESS;
      LogicBlock* logic_block = BlockFileManager::get_instance()->get_logic_block(fileid,chunkid);
      if (NULL == logic_block)
      {
        TBSYS_LOG(TFWARN, "blockid:<%" PRIu64 ",%u> is not exist,try to create a new block.", fileid,chunkid);
        if(TFS_SUCCESS != new_single_block(fileid,chunkid))
             return TFS_ERROR;
        else
            logic_block = BlockFileManager::get_instance()->get_logic_block(fileid,chunkid);
      }
      ret = logic_block->write_raw_data(data_buffer, msg_len, data_offset);
      if (TFS_SUCCESS != ret)
      {
        TBSYS_LOG(TFERROR, "write data batch error. blockid:<%" PRIu64 ",%u>, ret: %d", fileid,chunkid, ret);
        return ret;
      }

      return TFS_SUCCESS;
    }

    int DataManagement::add_new_expire_block(const VSTRING* expire_block_ids, const VSTRING* remove_block_ids, const VSTRING* new_block_ids)
    {
        /*
      // delete expire block
      if (NULL != expire_block_ids)
      {
        TBSYS_LOG(INFO, "expire block list size: %u\n", static_cast<uint32_t>(expire_block_ids->size()));
        for (uint32_t i = 0; i < expire_block_ids->size(); ++i)
        {
          TBSYS_LOG(INFO, "expire(delete) block. blockid: %s\n", expire_block_ids->at(i).c_str());
          BlockFileManager::get_instance()->del_block(expire_block_ids->at(i));
        }
      }

      // delete remove block
      if (NULL != remove_block_ids)
      {
        TBSYS_LOG(INFO, "remove block list size: %u\n", static_cast<uint32_t>(remove_block_ids->size()));
        for (uint32_t i = 0; i < remove_block_ids->size(); ++i)
        {
          TBSYS_LOG(INFO, "delete block. blockid: %s\n", remove_block_ids->at(i).c_str());
          BlockFileManager::get_instance()->del_block(remove_block_ids->at(i));
        }
      }

      // new
      if (NULL != new_block_ids)
      {
        TBSYS_LOG(INFO, "new block list size: %u\n", static_cast<uint32_t>(new_block_ids->size()));
        for (uint32_t i = 0; i < new_block_ids->size(); ++i)
        {
          TBSYS_LOG(INFO, "new block. blockid: %s\n", new_block_ids->at(i).c_str());
          uint32_t physical_block_id = 0;
          BlockFileManager::get_instance()->new_block(new_block_ids->at(i), physical_block_id);
        }
      }*/

      return EXIT_SUCCESS;
    }

    // gc expired and no referenced datafile
    int DataManagement::gc_data_file()
    {
      int32_t current_time = time(NULL);
      int32_t diff_time = current_time - SYSPARAM_DATASERVER.expire_data_file_time_;

      if (last_gc_data_file_time_ < diff_time)
      {
        data_file_mutex_.lock();
        int32_t old_data_file_size = data_file_map_.size();
        for (DataFileMapIter it = data_file_map_.begin(); it != data_file_map_.end();)
        {
          // no reference and expire
          if (it->second && it->second->get_ref() <= 0 && it->second->get_last_update() < diff_time)
          {
            tbsys::gDelete(it->second);
            data_file_map_.erase(it++);
          }
          else
          {
            ++it;
          }
        }

        int32_t new_data_file_size = data_file_map_.size();

        last_gc_data_file_time_ = current_time;
        data_file_mutex_.unlock();
        TBSYS_LOG(TFINFO, "datafilemap size. old: %d, new: %d", old_data_file_size, new_data_file_size);
      }

      return TFS_SUCCESS;
    }

    // remove all datafile
    int DataManagement::remove_data_file()
    {
      data_file_mutex_.lock();
      for (DataFileMapIter it = data_file_map_.begin(); it != data_file_map_.end(); ++it)
      {
        tbsys::gDelete(it->second);
      }
      data_file_map_.clear();
      data_file_mutex_.unlock();
      return TFS_SUCCESS;
    }
    
    int DataManagement::show_block_status()
    {
      std::list<LogicBlock*> logic_blocks;
      std::list<LogicBlock*>::iterator lit;
      BlockFileManager::get_instance()->get_all_logic_block(logic_blocks);
      std::list<PhysicalBlock*>* phy_blocks;
      std::list<PhysicalBlock*>::iterator pit;
      
     for (lit = logic_blocks.begin(); lit != logic_blocks.end(); ++lit)
     {
           if (*lit)
           {
                printf("query logic block <%" PRIu64 ",%u> status\n", 
                               (*lit)->file_id(),(*lit)->chunk_id());
                phy_blocks = (*lit)->get_physic_block_list();
                std::vector < uint32_t > phy_block_ids;
                for (pit = phy_blocks->begin(); pit != phy_blocks->end(); ++pit)
                {
                    printf("    physical block %u contained>\n",(*pit)->get_physic_block_id());
                }
            }
      }
      printf("total logic block number:%u\n",logic_blocks.size());
      return TFS_SUCCESS;
    }
    
    void DataManagement::show_ds_filesystem_info()
    {
       
                                
        SuperBlock super_block_info;
        BlockFileManager::get_instance()->query_super_block(super_block_info);
        printf("mountpath:%s,main_block_count:%d used:%d size:%d,extend_block_count:%d,used:%d,size:%d\n",
                super_block_info.mount_point_,
                super_block_info.main_block_count_,super_block_info.used_block_count_,super_block_info.main_block_size_,
                super_block_info.extend_block_count_,super_block_info.used_extend_block_count_,super_block_info.extend_block_size_);
       
        int64_t used,total;
        BlockFileManager::get_instance()->query_space(used, total);
        printf("space used:%" PRIu64 ",total:%" PRIu64 "\n",used,total); 
        
        int32_t bit_map_len, normal_set_count,error_set_count;
        BlockFileManager::get_instance()->get_bit_map_info(bit_map_len,normal_set_count,error_set_count);
        printf("bitmap length: %u,normal_set_count:%u,error_set_count:%u\n",
                                bit_map_len,normal_set_count,error_set_count);
    }
    
  }
}
