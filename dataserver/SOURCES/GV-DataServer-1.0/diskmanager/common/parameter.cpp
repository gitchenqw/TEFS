/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: parameter.cpp 544 2011-06-23 02:32:20Z duanfei@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *
 */
 
#include "parameter.h"
#include <tbsys.h>
#include "config_item.h"
#include "error_msg.h"
#include "internal.h"
namespace
{
  const int PORT_PER_PROCESS = 2;
}
namespace tfs
{
  namespace common
  {
    FileSystemParameter FileSystemParameter::fs_parameter_;
    DataServerParameter DataServerParameter::ds_parameter_;

    int DataServerParameter::initialize(const std::string& index)
    {
        /*
      heart_interval_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_HEART_INTERVAL, 5);
      check_interval_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_CHECK_INTERVAL, 2);
      expire_data_file_time_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_EXPIRE_DATAFILE_TIME, 20);
      expire_cloned_block_time_
        = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_EXPIRE_CLONEDBLOCK_TIME, 300);
      expire_compact_time_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_EXPIRE_COMPACTBLOCK_TIME, 86400);
      replicate_thread_count_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_REPLICATE_THREADCOUNT, 3);
      //default use O_SYNC
      sync_flag_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_WRITE_SYNC_FLAG, 1);
      max_block_size_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_BLOCK_MAX_SIZE);
      dump_vs_interval_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_VISIT_STAT_INTERVAL, -1);

      const char* max_io_time = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_IO_WARN_TIME, "0");
      max_io_warn_time_ = strtoll(max_io_time, NULL, 10);
      if (max_io_warn_time_ < 200000 || max_io_warn_time_ > 2000000)
        max_io_warn_time_ = 1000000;

      tfs_backup_type_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_BACKUP_TYPE, 1);
      local_ns_ip_ = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_IP_ADDR);
      if (NULL == local_ns_ip_)
      {
        TBSYS_LOG(ERROR, "can not find %s in [%s]", CONF_IP_ADDR, CONF_SN_DATASERVER);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }
      local_ns_port_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_PORT);
      ns_addr_list_ = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_IP_ADDR_LIST);
      if (NULL == ns_addr_list_)
      {
        TBSYS_LOG(ERROR, "can not find %s in [%s]", CONF_IP_ADDR_LIST, CONF_SN_DATASERVER);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }
      slave_ns_ip_ = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_SLAVE_NSIP);
      //config_log_file_ = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_LOG_FILE);
      max_datafile_nums_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_DATA_FILE_NUMS, 50);
      max_crc_error_nums_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_MAX_CRCERROR_NUMS, 4);
      max_eio_error_nums_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_MAX_EIOERROR_NUMS, 6);
      expire_check_block_time_
        = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_EXPIRE_CHECKBLOCK_TIME, 86400);
      max_cpu_usage_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_MAX_CPU_USAGE, 60);
      dump_stat_info_interval_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_DUMP_STAT_INFO_INTERVAL, 60000000);
      return SYSPARAM_FILESYSPARAM.initialize(index);*/
      return 0;
    }

    std::string DataServerParameter::get_real_file_name(const std::string& src_file, 
        const std::string& index, const std::string& suffix)
    {
      return src_file + "_" + index + "." + suffix;
    }

    int DataServerParameter::get_real_ds_port(const int ds_port, const std::string& index)
    {
      return ds_port + ((atoi((index.c_str())) - 1) * PORT_PER_PROCESS);
    }

    int FileSystemParameter::initialize(const std::string& index)
    {
        /*
      if (TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_MOUNT_POINT_NAME) == NULL || strlen(TBSYS_CONFIG.getString(
              CONF_SN_DATASERVER, CONF_MOUNT_POINT_NAME)) >= static_cast<uint32_t> (MAX_DEV_NAME_LEN))
      {
        TBSYS_LOG(ERROR, "can not find %s in [%s]", CONF_MOUNT_POINT_NAME, CONF_SN_DATASERVER);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }

      mount_name_ = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_MOUNT_POINT_NAME);
      mount_name_ = get_real_mount_name(mount_name_, index);

      const char* tmp_max_size = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_MOUNT_MAX_USESIZE);
      if (tmp_max_size == NULL)
      {
        TBSYS_LOG(ERROR, "can not find %s in [%s]", CONF_MOUNT_MAX_USESIZE, CONF_SN_DATASERVER);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }

      max_mount_size_ = strtoull(tmp_max_size, NULL, 10);
      base_fs_type_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_BASE_FS_TYPE);
      super_block_reserve_offset_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_SUPERBLOCK_START, 0);
      avg_segment_size_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_AVG_SEGMENT_SIZE);
      main_block_size_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_MAINBLOCK_SIZE);
      extend_block_size_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_EXTBLOCK_SIZE);

      const char* tmp_ratio = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_BLOCKTYPE_RATIO);
      if (tmp_ratio == NULL)
      {
        TBSYS_LOG(ERROR, "can not find %s in [%s]", CONF_BLOCKTYPE_RATIO, CONF_SN_DATASERVER);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }

      block_type_ratio_ = strtof(tmp_ratio, NULL);
      if (block_type_ratio_ == 0)
      {
        TBSYS_LOG(ERROR, "%s error :%s", CONF_BLOCKTYPE_RATIO, tmp_ratio);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }

      file_system_version_ = TBSYS_CONFIG.getInt(CONF_SN_DATASERVER, CONF_BLOCK_VERSION, 1);

      const char* tmp_hash_ratio = TBSYS_CONFIG.getString(CONF_SN_DATASERVER, CONF_HASH_SLOT_RATIO);
      if (tmp_hash_ratio == NULL)
      {
        TBSYS_LOG(ERROR, "can not find %s in [%s]", CONF_HASH_SLOT_RATIO, CONF_SN_DATASERVER);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }

      hash_slot_ratio_ = strtof(tmp_hash_ratio, NULL);
      if (hash_slot_ratio_ == 0)
      {
        TBSYS_LOG(ERROR, "%s error :%s", CONF_HASH_SLOT_RATIO, tmp_hash_ratio);
        return EXIT_SYSTEM_PARAMETER_ERROR;
      }
        */
      return TFS_SUCCESS;
    }
    std::string FileSystemParameter::get_real_mount_name(const std::string& mount_name, const std::string& index)
    {
      return mount_name + index;
    }
  }/** common **/
}/** tfs **/

