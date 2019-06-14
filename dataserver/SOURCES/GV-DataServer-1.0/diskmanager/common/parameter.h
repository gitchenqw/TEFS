/*
 * (C) 2007-2010 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Version: $Id: parameter.h 544 2011-06-23 02:32:20Z duanfei@taobao.com $
 *
 * Authors:
 *   duolong <duolong@taobao.com>
 *      - initial release
 *
 */
#ifndef TFS_COMMON_PARAMETER_H_
#define TFS_COMMON_PARAMETER_H_

#include "internal.h"

namespace tfs
{
  namespace common
  {
    struct FileSystemParameter
    {
      int initialize(const std::string& index);
      std::string mount_name_; // name of mount point
      uint64_t max_mount_size_; // the max space of the mount point
      int base_fs_type_;
      int32_t super_block_reserve_offset_;
      int32_t avg_segment_size_;
      int32_t main_block_size_;
      int32_t extend_block_size_;
      float block_type_ratio_;
      int32_t file_system_version_;
      float hash_slot_ratio_; // 0.5
      static FileSystemParameter fs_parameter_;
      static std::string get_real_mount_name(const std::string& mount_name, const std::string& index);
      static FileSystemParameter& instance()
      {
        return fs_parameter_;
      }
    };

    struct DataServerParameter
    {
      int initialize(const std::string& index);
      int32_t heart_interval_;
      int32_t check_interval_;
      int32_t expire_data_file_time_;
      int32_t expire_cloned_block_time_;
      int32_t expire_compact_time_;
      int32_t replicate_thread_count_;
      int32_t max_block_size_;
      int32_t sync_flag_;
      int32_t dump_vs_interval_;
      int64_t max_io_warn_time_;
      int32_t tfs_backup_type_;
      const char* local_ns_ip_;
      int32_t local_ns_port_;
      const char* slave_ns_ip_;
      const char* ns_addr_list_;
      int32_t max_datafile_nums_;
      int32_t max_crc_error_nums_;
      int32_t max_eio_error_nums_;
      int32_t expire_check_block_time_;
      int32_t max_cpu_usage_;
      int32_t dump_stat_info_interval_;
      static std::string get_real_file_name(const std::string& src_file, 
          const std::string& index, const std::string& suffix);
      static int get_real_ds_port(const int ds_port, const std::string& index);
      static DataServerParameter ds_parameter_;
      static DataServerParameter& instance()
      {
        return ds_parameter_;
      }
    };

#define SYSPARAM_NAMESERVER NameServerParameter::instance()
#define SYSPARAM_DATASERVER DataServerParameter::instance()
#define SYSPARAM_FILESYSPARAM FileSystemParameter::instance()
#define SYSPARAM_RCSERVER RcServerParameter::instance()
  }/** common **/
}/** tfs **/
#endif //TFS_COMMON_SYSPARAM_H_
