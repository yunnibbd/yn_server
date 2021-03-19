#include "ccrypt.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#define WORD 32
#define MASK 0xFFFFFFFF
#if __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef uint32_t WORD32;
#else
typedef unsigned int WORD32;
#endif
using namespace std;

static int b64index(uint8_t c)
{
	static const int decoding[] = { 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
	int decoding_size = sizeof(decoding) / sizeof(decoding[0]);
	if (c < 43)
		return -1;
	c -= 43;
	if (c >= decoding_size)
		return -1;
	return decoding[c];
}

char *CCrypt::Base64Decode(const uint8_t* text, size_t sz, int* out_size)
{
	int decode_sz = (sz + 3) / 4 * 3;
	char *buffer = nullptr;
	if (decode_sz > SMALL_CHUNK)
		buffer = new char[decode_sz];
	else
		buffer = new char[SMALL_CHUNK];
	int i, j;
	int output = 0;
	for (i = 0; i < (int)sz; )
	{
		int padding = 0;
		int c[4];
		for (j = 0; j < 4;)
		{
			if (i >= (int)sz)
				goto failed;
			c[j] = b64index(text[i]);
			if (c[j] == -1)
			{
				++i;
				continue;
			}
			if (c[j] == -2)
				++padding;
			++i;
			++j;
		}
		uint32_t v;
		switch (padding)
		{
		case 0:
			v = (unsigned)c[0] << 18 | c[1] << 12 | c[2] << 6 | c[3];
			buffer[output] = v >> 16;
			buffer[output + 1] = (v >> 8) & 0xff;
			buffer[output + 2] = v & 0xff;
			output += 3;
			break;
		case 1:
			if (c[3] != -2 || (c[2] & 3) != 0)
				goto failed;
			v = (unsigned)c[0] << 10 | c[1] << 4 | c[2] >> 2;
			buffer[output] = v >> 8;
			buffer[output + 1] = v & 0xff;
			output += 2;
			break;
		case 2:
			if (c[3] != -2 || c[2] != -2 || (c[1] & 0xf) != 0)
				goto failed;
			v = (unsigned)c[0] << 2 | c[1] >> 4;
			buffer[output] = v;
			++output;
			break;
		default:
			goto failed;
		}
	}
	*out_size = output;
	return buffer;

failed:
	if (buffer)
		delete[] buffer;
	*out_size = 0;
	return nullptr;
}

char *CCrypt::Base64Encode(const uint8_t* text, int sz, int* out_esz)
{
	static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int encode_sz = (sz + 2) / 3 * 4;
	char *buffer = NULL;
	if (encode_sz >= SMALL_CHUNK)
		buffer = new char[encode_sz + 1];
	else
		buffer = new char[SMALL_CHUNK];
	int i, j;
	j = 0;
	for (i = 0; i < (int)sz - 2; i += 3)
	{
		uint32_t v = text[i] << 16 | text[i + 1] << 8 | text[i + 2];
		buffer[j] = encoding[v >> 18];
		buffer[j + 1] = encoding[(v >> 12) & 0x3f];
		buffer[j + 2] = encoding[(v >> 6) & 0x3f];
		buffer[j + 3] = encoding[(v) & 0x3f];
		j += 4;
	}
	int padding = sz - i;
	uint32_t v;
	switch (padding)
	{
	case 1:
		v = text[i];
		buffer[j] = encoding[v >> 2];
		buffer[j + 1] = encoding[(v & 3) << 4];
		buffer[j + 2] = '=';
		buffer[j + 3] = '=';
		break;
	case 2:
		v = text[i] << 8 | text[i + 1];
		buffer[j] = encoding[v >> 10];
		buffer[j + 1] = encoding[(v >> 4) & 0x3f];
		buffer[j + 2] = encoding[(v & 0xf) << 2];
		buffer[j + 3] = '=';
		break;
	}
	buffer[encode_sz] = 0;
	*out_esz = encode_sz;
	return buffer;
}

void CCrypt::Base64EncodeFree(char* result)
{
	delete[] result;
}

////////////////////////////////////////////////////////////////////////

typedef struct {
	uint32_t state[5];
	uint32_t count[2];
	uint8_t  buffer[64];
} SHA1_CTX;

static void	SHA1_Transform(uint32_t	state[5], const	uint8_t	buffer[64]);

#define	rol(value, bits) (((value) << (bits)) |	((value) >>	(32	- (bits))))

/* blk0() and blk()	perform	the	initial	expand.	*/
/* I got the idea of expanding during the round	function from SSLeay */
/* FIXME: can we do	this in	an endian-proof	way? */
#ifdef WORDS_BIGENDIAN
#define	blk0(i)	block.l[i]
#else
#define	blk0(i)	(block.l[i]	= (rol(block.l[i],24)&0xFF00FF00) \
	|(rol(block.l[i],8)&0x00FF00FF))
#endif
#define	blk(i) (block.l[i&15] =	rol(block.l[(i+13)&15]^block.l[(i+8)&15] \
	^block.l[(i+2)&15]^block.l[i&15],1))

/* (R0+R1),	R2,	R3,	R4 are the different operations	used in	SHA1 */
#define	R0(v,w,x,y,z,i)	z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define	R1(v,w,x,y,z,i)	z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define	R2(v,w,x,y,z,i)	z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define	R3(v,w,x,y,z,i)	z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define	R4(v,w,x,y,z,i)	z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


/* Hash	a single 512-bit block.	This is	the	core of	the	algorithm. */
static void	SHA1_Transform(uint32_t	state[5], const	uint8_t	buffer[64])
{
	uint32_t a, b, c, d, e;
	typedef	union {
		uint8_t	c[64];
		uint32_t l[16];
	} CHAR64LONG16;
	CHAR64LONG16 block;

	memcpy(&block, buffer, 64);

	/* Copy	context->state[] to	working	vars */
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	/* 4 rounds	of 20 operations each. Loop	unrolled. */
	R0(a, b, c, d, e, 0); R0(e, a, b, c, d, 1);	R0(d, e, a, b, c, 2); R0(c, d, e, a, b, 3);
	R0(b, c, d, e, a, 4); R0(a, b, c, d, e, 5);	R0(e, a, b, c, d, 6); R0(d, e, a, b, c, 7);
	R0(c, d, e, a, b, 8); R0(b, c, d, e, a, 9);	R0(a, b, c, d, e, 10); R0(e, a, b, c, d, 11);
	R0(d, e, a, b, c, 12); R0(c, d, e, a, b, 13);	R0(b, c, d, e, a, 14); R0(a, b, c, d, e, 15);
	R1(e, a, b, c, d, 16); R1(d, e, a, b, c, 17);	R1(c, d, e, a, b, 18); R1(b, c, d, e, a, 19);
	R2(a, b, c, d, e, 20); R2(e, a, b, c, d, 21);	R2(d, e, a, b, c, 22); R2(c, d, e, a, b, 23);
	R2(b, c, d, e, a, 24); R2(a, b, c, d, e, 25);	R2(e, a, b, c, d, 26); R2(d, e, a, b, c, 27);
	R2(c, d, e, a, b, 28); R2(b, c, d, e, a, 29);	R2(a, b, c, d, e, 30); R2(e, a, b, c, d, 31);
	R2(d, e, a, b, c, 32); R2(c, d, e, a, b, 33);	R2(b, c, d, e, a, 34); R2(a, b, c, d, e, 35);
	R2(e, a, b, c, d, 36); R2(d, e, a, b, c, 37);	R2(c, d, e, a, b, 38); R2(b, c, d, e, a, 39);
	R3(a, b, c, d, e, 40); R3(e, a, b, c, d, 41);	R3(d, e, a, b, c, 42); R3(c, d, e, a, b, 43);
	R3(b, c, d, e, a, 44); R3(a, b, c, d, e, 45);	R3(e, a, b, c, d, 46); R3(d, e, a, b, c, 47);
	R3(c, d, e, a, b, 48); R3(b, c, d, e, a, 49);	R3(a, b, c, d, e, 50); R3(e, a, b, c, d, 51);
	R3(d, e, a, b, c, 52); R3(c, d, e, a, b, 53);	R3(b, c, d, e, a, 54); R3(a, b, c, d, e, 55);
	R3(e, a, b, c, d, 56); R3(d, e, a, b, c, 57);	R3(c, d, e, a, b, 58); R3(b, c, d, e, a, 59);
	R4(a, b, c, d, e, 60); R4(e, a, b, c, d, 61);	R4(d, e, a, b, c, 62); R4(c, d, e, a, b, 63);
	R4(b, c, d, e, a, 64); R4(a, b, c, d, e, 65);	R4(e, a, b, c, d, 66); R4(d, e, a, b, c, 67);
	R4(c, d, e, a, b, 68); R4(b, c, d, e, a, 69);	R4(a, b, c, d, e, 70); R4(e, a, b, c, d, 71);
	R4(d, e, a, b, c, 72); R4(c, d, e, a, b, 73);	R4(b, c, d, e, a, 74); R4(a, b, c, d, e, 75);
	R4(e, a, b, c, d, 76); R4(d, e, a, b, c, 77);	R4(c, d, e, a, b, 78); R4(b, c, d, e, a, 79);

	/* Add the working vars	back into context.state[] */
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	/* Wipe	variables */
	a = b = c = d = e = 0;
}


/* SHA1Init	- Initialize new context */
static void sat_SHA1_Init(SHA1_CTX* context)
{
	/* SHA1	initialization constants */
	context->state[0] = 0x67452301;
	context->state[1] = 0xEFCDAB89;
	context->state[2] = 0x98BADCFE;
	context->state[3] = 0x10325476;
	context->state[4] = 0xC3D2E1F0;
	context->count[0] = context->count[1] = 0;
}


/* Run your	data through this. */
static void sat_SHA1_Update(SHA1_CTX* context, const uint8_t* data, const size_t len)
{
	size_t i, j;

#ifdef VERBOSE
	SHAPrintContext(context, "before");
#endif

	j = (context->count[0] >> 3) & 63;
	if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
	context->count[1] += (len >> 29);
	if ((j + len) > 63) {
		memcpy(&context->buffer[j], data, (i = 64 - j));
		SHA1_Transform(context->state, context->buffer);
		for (; i + 63 < len; i += 64) {
			SHA1_Transform(context->state, data + i);
		}
		j = 0;
	}
	else i = 0;
	memcpy(&context->buffer[j], &data[i], len - i);

#ifdef VERBOSE
	SHAPrintContext(context, "after	");
#endif
}


/* Add padding and return the message digest. */
static void sat_SHA1_Final(SHA1_CTX* context, uint8_t digest[SHA1_DIGEST_SIZE])
{
	uint32_t i;
	uint8_t	 finalcount[8];

	for (i = 0; i < 8; i++) {
		finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
			>> ((3 - (i & 3)) * 8)) & 255);	 /*	Endian independent */
	}
	sat_SHA1_Update(context, (uint8_t *)"\200", 1);
	while ((context->count[0] & 504) != 448) {
		sat_SHA1_Update(context, (uint8_t *)"\0", 1);
	}
	sat_SHA1_Update(context, finalcount, 8);  /* Should	cause a	SHA1_Transform() */
	for (i = 0; i < SHA1_DIGEST_SIZE; i++) {
		digest[i] = (uint8_t)
			((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
	}

	/* Wipe	variables */
	i = 0;
	memset(context->buffer, 0, 64);
	memset(context->state, 0, 20);
	memset(context->count, 0, 8);
	memset(finalcount, 0, 8);	/* SWR */
}


void CCrypt::Sha1(uint8_t* buffer, int sz, uint8_t* digest, int* e_sz) 
{
	SHA1_CTX ctx;
	sat_SHA1_Init(&ctx);
	sat_SHA1_Update(&ctx, buffer, sz);
	sat_SHA1_Final(&ctx, digest);

	*e_sz = SHA1_DIGEST_SIZE;
}

/////////////////////////////////////////////////////////////////////

/*
** Realiza a rotacao no sentido horario dos bits da variavel 'D' do tipo WORD32.
** Os bits sao deslocados de 'num' posicoes
*/
#define rotate(D, num)  (D<<num) | (D>>(WORD-num))

/*Macros que definem operacoes relizadas pelo algoritmo  md5 */
#define F(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~(z))))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~(z))))


/*vetor de numeros utilizados pelo algoritmo md5 para embaralhar bits */
static const WORD32 T[64] = {
					 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
					 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
					 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
					 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
					 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
					 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
					 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
					 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
					 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
					 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
					 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
					 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
					 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
					 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
					 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
					 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};


static void word32tobytes(const WORD32 *input, char *output) {
	int j = 0;
	while (j < 4 * 4) {
		WORD32 v = *input++;
		output[j++] = (char)(v & 0xff); v >>= 8;
		output[j++] = (char)(v & 0xff); v >>= 8;
		output[j++] = (char)(v & 0xff); v >>= 8;
		output[j++] = (char)(v & 0xff);
	}
}


static void inic_digest(WORD32 *d) {
	d[0] = 0x67452301;
	d[1] = 0xEFCDAB89;
	d[2] = 0x98BADCFE;
	d[3] = 0x10325476;
}


/*funcao que implemeta os quatro passos principais do algoritmo MD5 */
static void digest(const WORD32 *m, WORD32 *d) {
	int j;
	/*MD5 PASSO1 */
	for (j = 0; j < 4 * 4; j += 4) {
		d[0] = d[0] + F(d[1], d[2], d[3]) + m[j] + T[j];       d[0] = rotate(d[0], 7);
		d[0] += d[1];
		d[3] = d[3] + F(d[0], d[1], d[2]) + m[(j)+1] + T[j + 1]; d[3] = rotate(d[3], 12);
		d[3] += d[0];
		d[2] = d[2] + F(d[3], d[0], d[1]) + m[(j)+2] + T[j + 2]; d[2] = rotate(d[2], 17);
		d[2] += d[3];
		d[1] = d[1] + F(d[2], d[3], d[0]) + m[(j)+3] + T[j + 3]; d[1] = rotate(d[1], 22);
		d[1] += d[2];
	}
	/*MD5 PASSO2 */
	for (j = 0; j < 4 * 4; j += 4) {
		d[0] = d[0] + G(d[1], d[2], d[3]) + m[(5 * j + 1) & 0x0f] + T[(j - 1) + 17];
		d[0] = rotate(d[0], 5);
		d[0] += d[1];
		d[3] = d[3] + G(d[0], d[1], d[2]) + m[((5 * (j + 1) + 1) & 0x0f)] + T[(j + 0) + 17];
		d[3] = rotate(d[3], 9);
		d[3] += d[0];
		d[2] = d[2] + G(d[3], d[0], d[1]) + m[((5 * (j + 2) + 1) & 0x0f)] + T[(j + 1) + 17];
		d[2] = rotate(d[2], 14);
		d[2] += d[3];
		d[1] = d[1] + G(d[2], d[3], d[0]) + m[((5 * (j + 3) + 1) & 0x0f)] + T[(j + 2) + 17];
		d[1] = rotate(d[1], 20);
		d[1] += d[2];
	}
	/*MD5 PASSO3 */
	for (j = 0; j < 4 * 4; j += 4) {
		d[0] = d[0] + H(d[1], d[2], d[3]) + m[(3 * j + 5) & 0x0f] + T[(j - 1) + 33];
		d[0] = rotate(d[0], 4);
		d[0] += d[1];
		d[3] = d[3] + H(d[0], d[1], d[2]) + m[(3 * (j + 1) + 5) & 0x0f] + T[(j + 0) + 33];
		d[3] = rotate(d[3], 11);
		d[3] += d[0];
		d[2] = d[2] + H(d[3], d[0], d[1]) + m[(3 * (j + 2) + 5) & 0x0f] + T[(j + 1) + 33];
		d[2] = rotate(d[2], 16);
		d[2] += d[3];
		d[1] = d[1] + H(d[2], d[3], d[0]) + m[(3 * (j + 3) + 5) & 0x0f] + T[(j + 2) + 33];
		d[1] = rotate(d[1], 23);
		d[1] += d[2];
	}
	/*MD5 PASSO4 */
	for (j = 0; j < 4 * 4; j += 4) {
		d[0] = d[0] + I(d[1], d[2], d[3]) + m[(7 * j) & 0x0f] + T[(j - 1) + 49];
		d[0] = rotate(d[0], 6);
		d[0] += d[1];
		d[3] = d[3] + I(d[0], d[1], d[2]) + m[(7 * (j + 1)) & 0x0f] + T[(j + 0) + 49];
		d[3] = rotate(d[3], 10);
		d[3] += d[0];
		d[2] = d[2] + I(d[3], d[0], d[1]) + m[(7 * (j + 2)) & 0x0f] + T[(j + 1) + 49];
		d[2] = rotate(d[2], 15);
		d[2] += d[3];
		d[1] = d[1] + I(d[2], d[3], d[0]) + m[(7 * (j + 3)) & 0x0f] + T[(j + 2) + 49];
		d[1] = rotate(d[1], 21);
		d[1] += d[2];
	}
}


static void bytestoword32(WORD32 *x, const char *pt) {
	int i;
	for (i = 0; i < 16; i++) {
		int j = i * 4;
		x[i] = (((WORD32)(unsigned char)pt[j + 3] << 8 |
			(WORD32)(unsigned char)pt[j + 2]) << 8 |
			(WORD32)(unsigned char)pt[j + 1]) << 8 |
			(WORD32)(unsigned char)pt[j];
	}

}


static void put_length(WORD32 *x, long len) {
	/* in bits! */
	x[14] = (WORD32)((len << 3) & MASK);
	x[15] = (WORD32)(len >> (32 - 3) & 0x7);
}


/*
** returned status:
*  0 - normal message (full 64 bytes)
*  1 - enough room for 0x80, but not for message length (two 4-byte words)
*  2 - enough room for 0x80 plus message length (at least 9 bytes free)
*/
static int converte(WORD32 *x, const char *pt, int num, int old_status) {
	int new_status = 0;
	char buff[64];
	if (num < 64) {
		memcpy(buff, pt, num);  /* to avoid changing original string */
		memset(buff + num, 0, 64 - num);
		if (old_status == 0)
			buff[num] = '\200';
		new_status = 1;
		pt = buff;
	}
	bytestoword32(x, pt);
	if (num <= (64 - 9))
		new_status = 2;
	return new_status;
}

void CCrypt::Md5(const char *message, long len, char *output) 
{
	WORD32 d[4];
	int status = 0;
	long i = 0;
	inic_digest(d);
	while (status != 2) {
		WORD32 d_old[4];
		WORD32 wbuff[16];
		int numbytes = (len - i >= 64) ? 64 : len - i;
		/*salva os valores do vetor digest*/
		d_old[0] = d[0]; d_old[1] = d[1]; d_old[2] = d[2]; d_old[3] = d[3];
		status = converte(wbuff, message + i, numbytes, status);
		if (status == 2) put_length(wbuff, len);
		digest(wbuff, d);
		d[0] += d_old[0]; d[1] += d_old[1]; d[2] += d_old[2]; d[3] += d_old[3];
		i += numbytes;
	}
	word32tobytes(d, output);
}
