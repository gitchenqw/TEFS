#include <string.h>
#include <stdlib.h>
#include <openssl/aes.h>

#include "gvfs_base_type.h"
#include "gvfs_aes.h"


static VOID 
gvfs_aes_ecb_pkcs5padding(CHAR *src)
{
	INT32  i;
	size_t ssize;
	CHAR   psize;

	ssize = strlen(src);

	src += ssize;
	psize = GVFS_AES_BLOCK_SIZE - (ssize % GVFS_AES_BLOCK_SIZE);
	for (i = 0; i < psize; i++) {
		memcpy(src + i, &psize, sizeof(CHAR));
	}
}


INT32
gvfs_aes_ecb_encrypt(CHAR *src, UCHAR *dst, const CHAR *key)
{
	INT32    i;
	size_t   ssize, ksize;
	AES_KEY  akey;

	ksize = strlen(key);
	if (GVFS_AES_KEY_LEN16 != ksize && GVFS_AES_KEY_LEN24 != ksize &&
		GVFS_AES_KEY_LEN32 != ksize)
	{
		return GVFS_ERROR;
	}

	gvfs_aes_ecb_pkcs5padding(src);

	memset(&akey, 0, sizeof(AES_KEY));
	if (0 != AES_set_encrypt_key((UCHAR *) key, ksize * GVFS_BYTE_TO_BIT,
								  &akey))
	{
		return GVFS_ERROR;
	}

	ssize = strlen(src);
	for (i = 0; i < ssize; i += GVFS_AES_BLOCK_SIZE) {
		AES_ecb_encrypt((UCHAR *) (src + i), dst + i, &akey, AES_ENCRYPT);
	}

	return i;
}


INT32
gvfs_aes_ecb_decrypt(UCHAR *src, size_t ssize, CHAR *dst,
	const CHAR *key)
{
	INT32    i;
	size_t   ksize;
	AES_KEY  akey;

	ksize = strlen(key);
	if (GVFS_AES_KEY_LEN16 != ksize && GVFS_AES_KEY_LEN24 != ksize &&
		GVFS_AES_KEY_LEN32 != ksize)
	{
		return GVFS_ERROR;
	}

	memset(&akey, 0, sizeof(AES_KEY));
	if (0 != AES_set_decrypt_key((UCHAR *) key, ksize * GVFS_BYTE_TO_BIT,
								  &akey))
	{
		return GVFS_ERROR;
	}

	for (i = 0; i < ssize; i += GVFS_AES_BLOCK_SIZE) {
		AES_ecb_encrypt(src + i, (UCHAR *) (dst + i), &akey, AES_DECRYPT);
	}

	return i;
}

