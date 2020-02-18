#ifndef CRYPTO_H__
#define CRYPTO_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/sha.h>

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define AES_KEY_LENGTH 32

unsigned char* crypto_generate_key(char *token);

bool crypto_init(unsigned char *key_data,
			 	int key_data_len,
			 	unsigned char *salt,
             	unsigned char *key,
             	unsigned char *iv);

int crypto_encrypt(unsigned char *plaintext,
				   int plaintext_len,
				   unsigned char *key,
				   unsigned char *iv,
				   unsigned char *ciphertext);

int crypto_decrypt(unsigned char *ciphertext,
				   int ciphertext_len,
				   unsigned char *key,
				   unsigned char *iv,
				   unsigned char *plaintext);

#endif