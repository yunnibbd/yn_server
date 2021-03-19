#ifndef __CRYPT_H__
#define __CRYPT_H__
#include <stdint.h>
#include <iostream>

#define SMALL_CHUNK		 256
#define SHA1_DIGEST_SIZE 20
#define MD5_HASHSIZE     16

class CCrypt
{
public:
	static char *Base64Decode(const uint8_t* text, size_t sz, int* out_size);

	static char *Base64Encode(const uint8_t* text, int sz, int* out_esz);

	static void Base64EncodeFree(char* result);

	static void Sha1(uint8_t* buffer, int sz, uint8_t* output, int* e_sz);

	static void Md5(const char *message, long len, char *output);
};

#endif
