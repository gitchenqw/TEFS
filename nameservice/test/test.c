#include <gfs_config.h>
#include <gfs_busi_types.h>


#define ntohll(val) ((((unsigned long long)htonl((int)((val<<32)>>32)))<<32)|(unsigned int)htonl((int)(val>>32)))
#define htonll(val) ((((unsigned long long)htonl((int)((val<<32)>>32)))<<32)|(unsigned int)htonl((int)(val>>32)))


int gid = 10000;
char sendbuf[1024*11];
char recvbuf[1024*11];
int slen;
int rlen;


int setnonblock(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLET | EPOLLERR;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    setnonblock(fd);
}

void modfd(int epfd, int fd, int flag)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLET | EPOLLERR | flag;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}


void get_login(char *buf, int *len, int dsid)
{
    gfs_busi_req_login_t *login;
    gfs_uint64            remain;
    gfs_uint64            disk;

    remain = 1024;
    disk = 2048;

    login = (gfs_busi_req_login_t*)buf;
    login->head.id = htonl(10001);
    login->head.len = htonl(44);
    login->head.type = htonl(0x01);
    login->head.encrypt = htonl(0);

    login->dsid = htonl(dsid);  //1001
    login->remain = htonll(remain);
    login->disk = htonll(disk);
    login->type = htonl(1);
    login->separate = htonl(0);

    *len = 44;
}

void get_copyblock(char *buf, int *len)
{
    gfs_busi_nte_cpy_block_t *cpyblock;
    gfs_uint64            fid, remain;
    gfs_int32             bindex;

    fid = 2;
    bindex = 2;
    remain = 1024;

    cpyblock = (gfs_busi_nte_cpy_block_t*)buf;
    cpyblock->head.id = htonl(10001);
    cpyblock->head.len = htonl(40);
    cpyblock->head.type = htonl(0x08);
    cpyblock->head.encrypt = htonl(0);

    fid = ntohll(fid);
    bindex = ntohl(bindex);
    memcpy(cpyblock->bid, &fid, 8);
    memcpy(&cpyblock->bid[8], &bindex, 4);
    cpyblock->remain = htonll(remain);
    cpyblock->separate = htonl(0);

    *len = 40;
}

void get_delblock(char *buf, int *len)
{
    gfs_busi_nte_delblock_t *delblock;
    gfs_uint64            fid, remain;
    gfs_int32             bindex;

    fid = 2;
    bindex = 2;
    remain = 1024;

    delblock = (gfs_busi_nte_delblock_t*)buf;
    delblock->head.id = htonl(10001);
    delblock->head.len = htonl(40);
    delblock->head.type = htonl(0x06);
    delblock->head.encrypt = htonl(0);

    fid = ntohll(fid);
    bindex = ntohl(bindex);
    memcpy(delblock->bid, &fid, 8);
    memcpy(&delblock->bid[8], &bindex, 4);
    delblock->remain = htonll(remain);
    delblock->separate = htonl(0);

    *len = 40;
}

void get_delfile(char *buf, int *len)
{
    gfs_busi_req_delfile_t *delfile;
    gfs_uint64            fid, remain;
    gfs_int32             bindex;

    fid = 2;
    bindex = 2;
    remain = 1024;

    delfile = (gfs_busi_req_delfile_t*)buf;
    delfile->head.id = htonl(10001);
    delfile->head.len = htonl(36);
    delfile->head.type = htonl(0x04);
    delfile->head.encrypt = htonl(0);

    delfile->hash[0] = 0xb0;
    delfile->hash[1] = 0xcf;
    delfile->hash[2] = 0x69;
    delfile->hash[3] = 0x5d;
    delfile->hash[4] = 0x7e;
    delfile->hash[5] = 0xad;
    delfile->hash[6] = 0x26;
    delfile->hash[7] = 0x75;
    delfile->hash[8] = 0x6f;
    delfile->hash[9] = 0xb5;
    delfile->hash[10] = 0xb0;
    delfile->hash[11] = 0xc1;
    delfile->hash[12] = 0x40;
    delfile->hash[13] = 0x26;
    delfile->hash[14] = 0xf4;
    delfile->hash[15] = 0x73;

    delfile->separate = htonl(0);

    *len = 36;
}

void get_upload(char *buf, int *len, int bindex)
{
    gfs_busi_req_upload_t *upload;
    gfs_uint64            fsize;

    fsize = 1024;

    upload = (gfs_busi_req_upload_t*)buf;
    upload->head.id = htonl(10001);
    upload->head.len = htonl(52);
    upload->head.type = htonl(0x02);
    upload->head.encrypt = htonl(0);

    upload->hash[0] = 0xb0;
    upload->hash[1] = 0xcf;
    upload->hash[2] = 0x69;
    upload->hash[3] = 0x5d;
    upload->hash[4] = 0x7e;
    upload->hash[5] = 0xad;
    upload->hash[6] = 0x26;
    upload->hash[7] = 0x75;
    upload->hash[8] = 0x6f;
    upload->hash[9] = 0xb5;
    upload->hash[10] = 0xb0;
    upload->hash[11] = 0xc1;
    upload->hash[12] = 0x40;
    upload->hash[13] = 0x26;
    upload->hash[14] = 0xf4;
    upload->hash[15] = 0x73;

    upload->fsize = htonll(fsize);
    upload->bnum = htonl(4);
    upload->bindex = htonl(bindex);     //-1
    upload->separate = htonl(0);

    *len = 52;
}

void get_download(char *buf, int *len, int bindex)
{
    gfs_busi_req_download_t *download;
    gfs_uint64            fsize;

    fsize = 1024;

    download = (gfs_busi_req_download_t*)buf;
    download->head.id = htonl(10001);
    download->head.len = htonl(40);
    download->head.type = htonl(0x03);
    download->head.encrypt = htonl(0);

    download->hash[0] = 0x00;
    download->hash[1] = 0xcf;
    download->hash[2] = 0x69;
    download->hash[3] = 0x5d;
    download->hash[4] = 0x7e;
    download->hash[5] = 0xad;
    download->hash[6] = 0x26;
    download->hash[7] = 0x75;
    download->hash[8] = 0x6f;
    download->hash[9] = 0xb5;
    download->hash[10] = 0xb0;
    download->hash[11] = 0xc1;
    download->hash[12] = 0x40;
    download->hash[13] = 0x26;
    download->hash[14] = 0xf4;
    download->hash[15] = 0x72;

    download->bindex = htonl(bindex);     //-1
    download->separate = htonl(0);

    *len = 40;
}

void get_fsize(char *buf, int *len, int bindex)
{
    gfs_busi_req_getfsize_t *getfsize;

    getfsize = (gfs_busi_req_getfsize_t*)buf;
    getfsize->head.id = htonl(10001);
    getfsize->head.len = htonl(36);
    getfsize->head.type = htonl(0x0a);
    getfsize->head.encrypt = htonl(0);

    getfsize->hash[0] = 0x00;
    getfsize->hash[1] = 0xcf;
    getfsize->hash[2] = 0x69;
    getfsize->hash[3] = 0x5d;
    getfsize->hash[4] = 0x7e;
    getfsize->hash[5] = 0xad;
    getfsize->hash[6] = 0x26;
    getfsize->hash[7] = 0x75;
    getfsize->hash[8] = 0x6f;
    getfsize->hash[9] = 0xb5;
    getfsize->hash[10] = 0xb0;
    getfsize->hash[11] = 0xc1;
    getfsize->hash[12] = 0x40;
    getfsize->hash[13] = 0x26;
    getfsize->hash[14] = 0xf4;
    getfsize->hash[15] = 0x72;

    getfsize->separate = htonl(0);

    *len = 36;
}

int mysend(int fd, char *buf, int len)
{
    int ret, *req;
    ret = send(fd, buf, len, 0);
    if (ret < 0)
    {
        return -1;
    }
    req = (int*)buf;
    printf("[%d] send : len %d, id %d, type %d\n", fd, ntohl(req[0]), ntohl(req[1]), ntohl(req[2]));

    return 0;
}

int myrecv(int fd, char *buf, int len)
{
    int ret;
    int *rsp;
    char msg[64];
    int type, id, rtn, rlen;
    int remain;

    ret = recv(fd, buf, len, 0);
    if (ret < 0)
    {
        return -1;
    }
    else if (ret == 0)
    {
        return -1;
    }
    rsp = (int*)buf;
    rlen = ntohl(rsp[0]);
    id = ntohl(rsp[1]);
    type = ntohl(rsp[2]);
    rtn = ntohl(rsp[3]);
    printf("[%d] recv : len %d, id %d, type %d, rtn %d\n", fd, rlen, id, type, rtn);
    
    remain = rlen - ret;
    while(remain > 0)
    {
        ret = recv(fd, buf, remain, 0);
        remain -= ret;
    }
    

    if (type == 0x01) //type
    {
        slen = 0;
        //getusrlogin(sendbuf, &slen);
    }
    else if (type == 0x02)
    {
        slen = 0;
        //getforward(sendbuf, &slen);
        return 0;
    }
    else if (type == 0x04)
    {
        slen = 0;
        //getregister(sendbuf, &slen);
        return 0;
    }
    else if (type == 0x10)
    {
        return 0;
    }
    else if (type == 0x07)
    {
        //return -1;
    }

    return 0;
}

void start_conn(int epfd, int num, char *ip, int port)
{
    int ret, i;
    struct sockaddr_in addr;
    int fd;

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);

    for (i = 0; i < num; i++)
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            continue;

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
        {
            printf("%d : connect [%s:%d] successed...\n", i, ip, port);
            addfd(epfd, fd);
        }
    }
}

void close_conn(int epfd, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

int main(int argc, char **argv)
{
    int epfd, nums, i;
    int fd, ret;
    int len, param, dsid;


    param = atoi(argv[1]);
    epfd = epoll_create(100);
    if (epfd < 0)
    {
        printf("epoll_create failed! errno code is %d\n", errno);
        return -1;
    }

    start_conn(epfd, 1, "127.0.0.1", 8082);

    struct epoll_event events[1000];
    slen = 0;
    dsid = 1001;
    //get_login(sendbuf, &slen, param);
    get_fsize(sendbuf, &slen, param);

    while (1)
    {
        nums = epoll_wait(epfd, events, 1000, 500);
        if (nums < 0)
        {
            printf("epoll_wait failed! errno code is %d\n", errno);
            return -1;
        }
        else if (nums == 0)
            continue;
        
        for (i = 0; i < nums; i++)
        {
            fd = events[i].data.fd;
            if (events[i].events & EPOLLERR)
            {
                printf("connect closeed fd : %d; errno code is %d\n", fd, errno);
                close_conn(epfd, fd);
            }
            else if (events[i].events & EPOLLIN)
            {
                len = 1024*11;
                sleep(1);
                ret = myrecv(fd, recvbuf, len);
                if (ret == 0)
                    modfd(epfd, fd, EPOLLOUT);
                if (ret == -1)
                {
                    printf("connect closeed fd : %d; errno code is %d\n", fd, errno);
                    close_conn(epfd, fd);
                    continue;
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                //get_login(sendbuf, &slen, dsid++);
                ret = mysend(fd, sendbuf, slen);
                if (ret)
                {
                    printf("send error! errno code is %d", errno);
                    continue;
                }
                modfd(epfd, fd, EPOLLIN);
            }
        }
    }

    return 0;
}

