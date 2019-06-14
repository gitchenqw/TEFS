
/*
 * test block manager
 *
 */
#include "blockfile_manager.h"
#include <unistd.h>
#include <stdio.h>

using namespace tfs::dataserver;
using namespace tfs::common;
using namespace std;
static int32_t failed = 0;
static BlockFileManager * blockfile_manager = NULL;
void EXPECT_EQ(int64_t a,int64_t b){assert(a==b);}
void EXPECT_NE(int64_t a,int64_t b){assert(a!=b);}
void EXPECT_TRUE(int a){assert(a);}
void EXPECT_FALSE(int a){assert(!a);}
void ADD_FAILURE(){ failed++;}

int set_fs_param(FileSystemParameter &fs_param)
{
  fs_param.mount_name_.assign("./disk");
  fs_param.max_mount_size_ = 20971520;
  fs_param.base_fs_type_ = 3;
  fs_param.super_block_reserve_offset_ = 0;
  fs_param.main_block_size_ = 16777216*4;    //64M
  fs_param.extend_block_size_ = 1048576;   //1M
  fs_param.block_type_ratio_ = 0.5;
  fs_param.file_system_version_ = 1;
  fs_param.hash_slot_ratio_ = 0.5;
  return 0;
}

int main(int argc, char* argv[])
{
    FileSystemParameter fs_param;
    blockfile_manager = BlockFileManager::get_instance();
    set_fs_param(fs_param);
    blockfile_manager->format_block_file_system(fs_param);  //format blockfile
   return 0;
}
