

#ifndef _GVFS_AES_H_INCLUDE_
#define _GVFS_AES_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define GVFS_BYTE_TO_BIT     8
#define GVFS_AES_BLOCK_SIZE  16

typedef enum gvfs_aes_key_len_e
{
	GVFS_AES_KEY_LEN16 = 16,
	GVFS_AES_KEY_LEN24 = 24,
	GVFS_AES_KEY_LEN32 = 32
} gvfs_aes_key_len_t;


INT32
gvfs_aes_ecb_encrypt(CHAR *src, UCHAR *dst, const CHAR *key);

INT32
gvfs_aes_ecb_decrypt(UCHAR *src, size_t ssize, CHAR *dst,
	const CHAR *key);


#endif /* _GVFS_AES_H_INCLUDE_ */

