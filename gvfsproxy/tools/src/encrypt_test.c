#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "gvfs_base_type.h"
#include "gvfs_aes.h"
#include "gvfs_base64.h"
#include "gvfs_zlib.h"

static long read_n(FILE *fp,char *buf,long n)
{
    long r,rc;

    r = 0;
    while (n > 0) {
        rc = fread(buf+r,1,n,fp);

        if (rc <= 0) {
            break;
        }

        r += rc;
        n -= r;
    }

    return r;
}


int main(int argc, char **argv)
{
	FILE *fp;
	CHAR *buf;
	CHAR *infile;
	CHAR *outfile;
	CHAR *key = "1qaz!QAZ1qaz!QAZ";
	long  size, r;

	infile = argv[1];
	outfile = argv[2];
	
    fp = fopen(infile,"rb");
    if (fp == NULL) {
        printf("fopen(\"%s\") failed, error: %s\n", infile, strerror(errno));
        return -1;
    }

    fseek(fp,0,SEEK_END);
    size = ftell(fp);

    if (size <= 0) {
    	printf("file:%s size:%ld error\n", infile, size);
        return -1;
    }

    buf = malloc(size + 1 + 16);
    if (buf == NULL) {
    	printf("malloc:%ld failed\n", size);
        return -1;
    }

    rewind(fp);

    r = read_n(fp, buf, size);
    if (r < size) {
    	printf("read:%ld return:%ld error\n", size, r);
        return -1;
    }
    buf[size] = '\0';
	

	//printf("before aes src:[%s] len:[%lu]\n", buf, size);
	
	UCHAR edst[1024] = {0};
	size_t esize;	
	esize = gvfs_aes_ecb_encrypt(buf, edst, key);
	if (0 > esize) {
		return -1;
	}
	//printf("after aes: len:[%lu]\n", esize);
	

	UCHAR cdst[1024] = {0};
	size_t cdlen = 1024;
	if (GVFS_OK != gvfs_compress(cdst, &cdlen, edst, esize)) {
		printf("gvfs_compress() failed\n");
		return -1;
	}
	//printf("after compress: len[%lu]\n", cdlen);

	UCHAR ebdst[1024] = {0};
	size_t ebdlen;
	gvfs_encode_base64(ebdst, &ebdlen, cdst, cdlen);

	//printf("after encode base64: ouput:[%s] len[%lu]\n", ebdst, ebdlen);

	//*(ebdst + ebdlen) = '\n';
	//ebdlen += 1;
	
	fclose(fp);
	free(buf);
	
    fp = fopen(outfile, "w+");
    if (fp == NULL) {
        printf("fopen(\"%s\") failed, error: %s\n", outfile, strerror(errno));
        return -1;
    }

	
	fwrite(ebdst, 1, ebdlen, fp);
	fclose(fp);
	
	return 0;
}

