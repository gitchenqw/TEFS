#include "data_management.h"
#include "blockfile_manager.h"
#include "common/directory_op.h"
#include "tbsys.h"
#include <string.h>
#include <string.h>

using namespace tfs::dataserver;
using namespace tfs::common;
using namespace std;
static DataManagement * data_manager = NULL;

// delete test files and folders
int clearfile(SuperBlock super_block)
{
  char str[16];
  chdir("./disk");
  for (int32_t i = 1; i <= super_block.main_block_count_; i++)
  {
    sprintf(str, "%d", i);
    unlink(str);
  }
  chdir("./extend");
  for (int32_t i = super_block.main_block_count_ + 1; i <= super_block.main_block_count_
      + super_block.extend_block_count_; i++)
  chdir("./index");
  for (int32_t i = 1; i <= super_block.main_block_count_; i++)
  {
    sprintf(str, "%d", i);
    unlink(str);
  }
  chdir("../");
  unlink("fs_super");
  rmdir("index");
  rmdir("extend");
  chdir("../");
  rmdir("disk");
  return 0;
}

int set_fs_param(FileSystemParameter &fs_param)
{
  fs_param.mount_name_.assign("./disk");
  fs_param.max_mount_size_ = 20971520;
  fs_param.base_fs_type_ = 3;
  fs_param.super_block_reserve_offset_ = 0;
  fs_param.avg_segment_size_ = 15 * 1024;  //15K
  fs_param.main_block_size_ = 16777216;    //16M
  fs_param.extend_block_size_ = 1048576;   //1M
  fs_param.block_type_ratio_ = 0.5;
  fs_param.file_system_version_ = 1;
  fs_param.hash_slot_ratio_ = 0.5;
  return 0;
}

int main(int argc, char* argv[])
{
    SuperBlock super_block;
     FileSystemParameter fs_param;
    set_fs_param(fs_param);
    //data_manager = new DataManagement();
    //data_manager->init_block_files(fs_param);
    BlockFileManager::get_instance()->clear_block_file_system(fs_param);
}