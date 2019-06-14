CC = gcc
CFLAGS = -pipe -O -Wall -Wpointer-arith -Wno-unused-parameter -Wunused-function -Wunused-variable -Wunused-value -g
LINK = $(CC)

ALL_INCS = -I src
	
SRCS = src/gvfs_aes.c \
	src/gvfs_zlib.c \
	src/gvfs_base64.c \

ENC_SRC = src/encrypt_test.c

DEC_SRC = src/decrypt_test.c


			