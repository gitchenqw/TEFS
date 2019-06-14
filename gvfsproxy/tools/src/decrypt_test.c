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
	

	//printf("before decode base64 src:[%s] len:[%lu]\n", buf, size);
	
	/* 下面开始解密流程 */
	UCHAR dbdst[1024] = {0};
	size_t dbdlen;
	gvfs_decode_base64(dbdst, &dbdlen, (UCHAR *) buf, size);
	//printf("after decode base64: len[%lu]\n", dbdlen);


	UCHAR ucdst[1024] = {0};
	size_t ucdlen = 1024;
	if (GVFS_OK != gvfs_uncompress(ucdst, &ucdlen, dbdst, dbdlen)) {
		printf("gvfs_uncompress() failed\n");
		return -1;
	}
	//printf("after uncompress: len[%lu]\n", ucdlen);

	CHAR final[1024] = {0};
	size_t dsize;
	dsize = gvfs_aes_ecb_decrypt(ucdst, ucdlen, final, key);
	if (0 > dsize) {
		return -1;
	}

	char last_value = *(final + dsize -1);
	final[dsize - last_value] = 0;

	//printf("final[%s]\n", final);
	
	fclose(fp);
	free(buf);
	
    fp = fopen(outfile, "w+");
    if (fp == NULL) {
        printf("fopen(\"%s\") failed, error: %s\n", outfile, strerror(errno));
        return -1;
    }

	
	fwrite(final, 1, dsize - last_value, fp);
	fclose(fp);
	

	
	return 0;
}

