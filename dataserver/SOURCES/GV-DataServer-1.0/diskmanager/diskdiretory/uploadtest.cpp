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
    if(argc != 4)
    {
        printf("./PUT <filename> <fileid> <chunkid>\n");
        return 0;
    }
    uint64_t fileid = atoi(argv[2]);
    uint32_t chunkid = atoi(argv[3]);
    FileSystemParameter fs_param;
    set_fs_param(fs_param);
    data_manager = new DataManagement();
    int ret = data_manager->init_block_files(fs_param);
    if(ret != TFS_SUCCESS)
        return 0;
    FileOperation rfile(argv[1],O_RDONLY);
    if(rfile.open_file() < 0)
    {
        printf("open file %s error\n",argv[1]);
        return 0;
    }
    char data_buffer[1024];
    int offset = 0;
    int length = 1024;
    while(offset < rfile.get_file_size())
    {
        if(rfile.get_file_size()-offset < 1024)
            length = rfile.get_file_size()-offset;
       if(TFS_SUCCESS != rfile.pread_file(data_buffer,length,offset))
           break;
        ret = data_manager->write_raw_data(fileid,chunkid, offset, length,data_buffer);
        if(ret != TFS_SUCCESS)
            break;
        data_manager->write_raw_data(fileid,chunkid, offset, length,data_buffer);
        offset += 1024;
    }
    rfile.close_file();
    delete data_manager; 
    if(ret == TFS_SUCCESS)
        printf("upload %s success\n",argv[1]);
    return 0;
}