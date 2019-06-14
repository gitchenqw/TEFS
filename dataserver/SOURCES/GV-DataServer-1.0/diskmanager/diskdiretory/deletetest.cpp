#include "data_management.h"
#include "common/directory_op.h"
#include "tbsys.h"
#include <string.h>

using namespace tfs::dataserver;
using namespace tfs::common;
using namespace std;
static DataManagement * data_manager = NULL;

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
    if(argc != 3)
    {
        printf("./DEL <fileid> <chunkid>\n");
        return 0;
    }
    uint64_t fileid;
    if(tfs::atoi64(argv[1],strlen(argv[1]),fileid) < 0)
    {    printf("invalid fileid\n");
        return -1;
    }
    uint32_t chunkid = atoi(argv[2]);
    FileSystemParameter fs_param;
    set_fs_param(fs_param);
    data_manager = new DataManagement();
    data_manager->init_block_files(fs_param);
    data_manager->del_single_block(fileid,chunkid);
    delete data_manager;
    return 0;
}