#include "gd32f3x0_it.h"
#include "gw_defines.h"

extern void SystemInit();
extern void main();
extern void do_led_glow();

static const uint32_t aes_rcon[10] = {
		0x01000000, 0x02000000, 0x04000000, 0x08000000,
		0x10000000, 0x20000000, 0x40000000, 0x80000000,
		0x1B000000, 0x36000000
};

static const uint32_t aes_te[256] = {
	0xc66363a5U, 0xf87c7c84U, 0xee777799U, 0xf67b7b8dU,
	0xfff2f20dU, 0xd66b6bbdU, 0xde6f6fb1U, 0x91c5c554U,
	0x60303050U, 0x02010103U, 0xce6767a9U, 0x562b2b7dU,
	0xe7fefe19U, 0xb5d7d762U, 0x4dababe6U, 0xec76769aU,
	0x8fcaca45U, 0x1f82829dU, 0x89c9c940U, 0xfa7d7d87U,
	0xeffafa15U, 0xb25959ebU, 0x8e4747c9U, 0xfbf0f00bU,
	0x41adadecU, 0xb3d4d467U, 0x5fa2a2fdU, 0x45afafeaU,
	0x239c9cbfU, 0x53a4a4f7U, 0xe4727296U, 0x9bc0c05bU,
	0x75b7b7c2U, 0xe1fdfd1cU, 0x3d9393aeU, 0x4c26266aU,
	0x6c36365aU, 0x7e3f3f41U, 0xf5f7f702U, 0x83cccc4fU,
	0x6834345cU, 0x51a5a5f4U, 0xd1e5e534U, 0xf9f1f108U,
	0xe2717193U, 0xabd8d873U, 0x62313153U, 0x2a15153fU,
	0x0804040cU, 0x95c7c752U, 0x46232365U, 0x9dc3c35eU,
	0x30181828U, 0x379696a1U, 0x0a05050fU, 0x2f9a9ab5U,
	0x0e070709U, 0x24121236U, 0x1b80809bU, 0xdfe2e23dU,
	0xcdebeb26U, 0x4e272769U, 0x7fb2b2cdU, 0xea75759fU,
	0x1209091bU, 0x1d83839eU, 0x582c2c74U, 0x341a1a2eU,
	0x361b1b2dU, 0xdc6e6eb2U, 0xb45a5aeeU, 0x5ba0a0fbU,
	0xa45252f6U, 0x763b3b4dU, 0xb7d6d661U, 0x7db3b3ceU,
	0x5229297bU, 0xdde3e33eU, 0x5e2f2f71U, 0x13848497U,
	0xa65353f5U, 0xb9d1d168U, 0x00000000U, 0xc1eded2cU,
	0x40202060U, 0xe3fcfc1fU, 0x79b1b1c8U, 0xb65b5bedU,
	0xd46a6abeU, 0x8dcbcb46U, 0x67bebed9U, 0x7239394bU,
	0x944a4adeU, 0x984c4cd4U, 0xb05858e8U, 0x85cfcf4aU,
	0xbbd0d06bU, 0xc5efef2aU, 0x4faaaae5U, 0xedfbfb16U,
	0x864343c5U, 0x9a4d4dd7U, 0x66333355U, 0x11858594U,
	0x8a4545cfU, 0xe9f9f910U, 0x04020206U, 0xfe7f7f81U,
	0xa05050f0U, 0x783c3c44U, 0x259f9fbaU, 0x4ba8a8e3U,
	0xa25151f3U, 0x5da3a3feU, 0x804040c0U, 0x058f8f8aU,
	0x3f9292adU, 0x219d9dbcU, 0x70383848U, 0xf1f5f504U,
	0x63bcbcdfU, 0x77b6b6c1U, 0xafdada75U, 0x42212163U,
	0x20101030U, 0xe5ffff1aU, 0xfdf3f30eU, 0xbfd2d26dU,
	0x81cdcd4cU, 0x180c0c14U, 0x26131335U, 0xc3ecec2fU,
	0xbe5f5fe1U, 0x359797a2U, 0x884444ccU, 0x2e171739U,
	0x93c4c457U, 0x55a7a7f2U, 0xfc7e7e82U, 0x7a3d3d47U,
	0xc86464acU, 0xba5d5de7U, 0x3219192bU, 0xe6737395U,
	0xc06060a0U, 0x19818198U, 0x9e4f4fd1U, 0xa3dcdc7fU,
	0x44222266U, 0x542a2a7eU, 0x3b9090abU, 0x0b888883U,
	0x8c4646caU, 0xc7eeee29U, 0x6bb8b8d3U, 0x2814143cU,
	0xa7dede79U, 0xbc5e5ee2U, 0x160b0b1dU, 0xaddbdb76U,
	0xdbe0e03bU, 0x64323256U, 0x743a3a4eU, 0x140a0a1eU,
	0x924949dbU, 0x0c06060aU, 0x4824246cU, 0xb85c5ce4U,
	0x9fc2c25dU, 0xbdd3d36eU, 0x43acacefU, 0xc46262a6U,
	0x399191a8U, 0x319595a4U, 0xd3e4e437U, 0xf279798bU,
	0xd5e7e732U, 0x8bc8c843U, 0x6e373759U, 0xda6d6db7U,
	0x018d8d8cU, 0xb1d5d564U, 0x9c4e4ed2U, 0x49a9a9e0U,
	0xd86c6cb4U, 0xac5656faU, 0xf3f4f407U, 0xcfeaea25U,
	0xca6565afU, 0xf47a7a8eU, 0x47aeaee9U, 0x10080818U,
	0x6fbabad5U, 0xf0787888U, 0x4a25256fU, 0x5c2e2e72U,
	0x381c1c24U, 0x57a6a6f1U, 0x73b4b4c7U, 0x97c6c651U,
	0xcbe8e823U, 0xa1dddd7cU, 0xe874749cU, 0x3e1f1f21U,
	0x964b4bddU, 0x61bdbddcU, 0x0d8b8b86U, 0x0f8a8a85U,
	0xe0707090U, 0x7c3e3e42U, 0x71b5b5c4U, 0xcc6666aaU,
	0x904848d8U, 0x06030305U, 0xf7f6f601U, 0x1c0e0e12U,
	0xc26161a3U, 0x6a35355fU, 0xae5757f9U, 0x69b9b9d0U,
	0x17868691U, 0x99c1c158U, 0x3a1d1d27U, 0x279e9eb9U,
	0xd9e1e138U, 0xebf8f813U, 0x2b9898b3U, 0x22111133U,
	0xd26969bbU, 0xa9d9d970U, 0x078e8e89U, 0x339494a7U,
	0x2d9b9bb6U, 0x3c1e1e22U, 0x15878792U, 0xc9e9e920U,
	0x87cece49U, 0xaa5555ffU, 0x50282878U, 0xa5dfdf7aU,
	0x038c8c8fU, 0x59a1a1f8U, 0x09898980U, 0x1a0d0d17U,
	0x65bfbfdaU, 0xd7e6e631U, 0x844242c6U, 0xd06868b8U,
	0x824141c3U, 0x299999b0U, 0x5a2d2d77U, 0x1e0f0f11U,
	0x7bb0b0cbU, 0xa85454fcU, 0x6dbbbbd6U, 0x2c16163aU,
};

static const uint8_t aes_sbox[256] =
{
	0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01,
	0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D,
	0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4,
	0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,
	0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15, 0x04, 0xC7,
	0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
	0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E,
	0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
	0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB,
	0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF, 0xD0, 0xEF, 0xAA, 0xFB,
	0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C,
	0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
	0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C,
	0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D,
	0x64, 0x5D, 0x19, 0x73, 0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A,
	0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
	0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3,
	0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
	0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A,
	0xAE, 0x08, 0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,
	0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E,
	0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9,
	0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9,
	0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
	0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99,
	0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

typedef struct _aes128_ctx_s
{
	uint32_t rounds;
	uint32_t tmp;
	uint32_t rk[11][4];
} aes128_ctx_s;

/*!
	\brief	  this function handles timer13 interrupts
	\param[in]  none
	\param[out] none
	\retval	 none
*/
void timer13_irq_handler(void)
{
	do_led_glow();
}

/*!
	\brief	  this function handles usbfs interrupts
	\param[in]  none
	\param[out] none
	\retval	 none
*/
void usbfs_irq_handler(void)
{
	// call usbfs irq of the bootloader
	((irq_handler_t)BLDR_USBFS_IRQ)(); 
}

int memcmp ( const void* _Ptr1, const void* _Ptr2, uint32_t _Size )
{
	for(uint32_t i = 0; i < _Size; ++i)
	{
		if (((uint8_t*)_Ptr1)[i] != ((uint8_t*)_Ptr2)[i])
			return ((uint8_t*)_Ptr1)[i] > ((uint8_t*)_Ptr2)[i] ? -i : i;
	}

	return 0;
}

void* memcpy ( void* _Dst, const void* _Src, uint32_t _Size )
{
	for(uint32_t i = 0; i < _Size; ++i)
		((uint8_t*)_Dst)[i] = ((uint8_t*)_Src)[i];

	return _Dst;
}

void* memset ( void* _Dst, int _Val, uint32_t _Size )
{
	for(uint32_t i = 0; i < _Size; ++i)
		((uint8_t*)_Dst)[i] = (uint8_t)_Val;

	return _Dst;
}

uint32_t rotate_right(uint32_t _value, uint32_t _amount)
{
	uint32_t value = _value;

	for(int i = 0; i < _amount / 8; ++i)
		value = ((value >> 8) & 0x00FFFFFF) | (uint32_t)((uint8_t)value << 24);

	return value;
}

void aes128_cipher(const uint8_t *_key, uint8_t *_data)
{
	aes128_ctx_s ctx;

	// load key
	for ( int i = 0; i != 4; ++i )
		ctx.rk[0][i] = __builtin_bswap32((uint32_t)&_key[i * sizeof(uint32_t)]);

	// expand key
	for ( int i = 0; i < 10; ++i )
	{
		uint32_t temp = ctx.rk[i][3];

		ctx.rk[i + 1][0] = ctx.rk[i][0] ^
					(aes_sbox[(uint8_t)(temp >> 16)] & 0xff000000) ^
					(aes_sbox[(uint8_t)(temp >>  8)] & 0x00ff0000) ^
					(aes_sbox[(uint8_t)(temp      )] & 0x0000ff00) ^
					(aes_sbox[(uint8_t)(temp >> 24)] & 0x000000ff) ^
					aes_rcon[i];

		ctx.rk[i + 1][1] = ctx.rk[i][1] ^ ctx.rk[i + 1][0];
		ctx.rk[i + 1][2] = ctx.rk[i][2] ^ ctx.rk[i + 1][1];
		ctx.rk[i + 1][3] = ctx.rk[i][3] ^ ctx.rk[i + 1][2];
	}

	uint32_t s0 = __builtin_bswap32((uint32_t)&_data[0]) ^ ctx.rk[0][0];
	uint32_t s1 = __builtin_bswap32((uint32_t)&_data[4]) ^ ctx.rk[0][1];
	uint32_t s2 = __builtin_bswap32((uint32_t)&_data[8]) ^ ctx.rk[0][2];
	uint32_t s3 = __builtin_bswap32((uint32_t)&_data[12]) ^ ctx.rk[0][3];

	// transform
	for( ctx.rounds = 1; ctx.rounds != 10; ++ctx.rounds)
	{
		uint32_t t0 = ctx.rk[ctx.rounds][0] ^
					  aes_te[(uint8_t)(s0 >> 24)] ^
					  rotate_right(aes_te[(uint8_t)(s1 >> 16)], 8) ^
					  rotate_right(aes_te[(uint8_t)(s2 >> 8)], 16) ^
					  rotate_right(aes_te[(uint8_t)s3], 24);

		uint32_t t1 = ctx.rk[ctx.rounds][1] ^
					  aes_te[(uint8_t)(s1 >> 24)] ^
					  rotate_right(aes_te[(uint8_t)(s2 >> 16)], 8) ^
					  rotate_right(aes_te[(uint8_t)(s3 >> 8)], 16) ^
					  rotate_right(aes_te[(uint8_t)s0], 24);

		uint32_t t2 = ctx.rk[ctx.rounds][2] ^
					  aes_te[(uint8_t)(s2 >> 24)] ^
					  rotate_right(aes_te[(uint8_t)(s3 >> 16)], 8) ^
					  rotate_right(aes_te[(uint8_t)(s0 >> 8)], 16) ^
					  rotate_right(aes_te[(uint8_t)s1], 24);

		uint32_t t3 = ctx.rk[ctx.rounds][3] ^
					  aes_te[(uint8_t)(s3 >> 24)] ^
					  rotate_right(aes_te[(uint8_t)(s0 >> 16)], 8) ^
					  rotate_right(aes_te[(uint8_t)(s1 >> 8)], 16) ^
					  rotate_right(aes_te[(uint8_t)s2], 24);

		s0 = t0;
		s1 = t1;
		s2 = t2;
		s3 = t3;
	}

	uint32_t v44 = ((aes_sbox[(uint8_t)(s0 >> 24)] << 24) | (aes_sbox[(uint8_t)(s1 >> 16)] << 16) | (aes_sbox[(uint8_t)(s2 >> 8)] << 8) | aes_sbox[(uint8_t)s3]) ^ ctx.rk[10][0];
	uint32_t v46 = ((aes_sbox[(uint8_t)(s1 >> 24)] << 24) | (aes_sbox[(uint8_t)(s2 >> 16)] << 16) | (aes_sbox[(uint8_t)(s3 >> 8)] << 8) | aes_sbox[(uint8_t)s0]) ^ ctx.rk[10][1];
	uint32_t v48 = ((aes_sbox[(uint8_t)(s2 >> 24)] << 24) | (aes_sbox[(uint8_t)(s3 >> 16)] << 16) | (aes_sbox[(uint8_t)(s0 >> 8)] << 8) | aes_sbox[(uint8_t)s1]) ^ ctx.rk[10][2];
	uint32_t v50 = ((aes_sbox[(uint8_t)(s3 >> 24)] << 24) | (aes_sbox[(uint8_t)(s0 >> 16)] << 16) | (aes_sbox[(uint8_t)(s1 >> 8)] << 8) | aes_sbox[(uint8_t)s2]) ^ ctx.rk[10][3];

	_data[0] = (uint8_t)(v44 >> 24);
	_data[1] = (uint8_t)(v44 >> 16);
	_data[2] = (uint8_t)(v44 >> 8);
	_data[3] = (uint8_t)v44;

	_data[4] = (uint8_t)(v46 >> 24);
	_data[5] = (uint8_t)(v46 >> 16);
	_data[6] = (uint8_t)(v46 >> 8);
	_data[7] = (uint8_t)v46;

	_data[8] = (uint8_t)(v48 >> 24);
	_data[9] = (uint8_t)(v48 >> 16);
	_data[10] = (uint8_t)(v48 >> 8);
	_data[11] = (uint8_t)v48;

	_data[12] = (uint8_t)(v50 >> 24);
	_data[13] = (uint8_t)(v50 >> 16);
	_data[14] = (uint8_t)(v50 >> 8);
	_data[15] = (uint8_t)v50;
}

// taken with love from: https://github.com/mpaland/printf

// 'ntoa' conversion buffer size, this must be big enough to hold one converted
// numeric number including padded zeros (dynamically created on stack)
// default: 32 byte
#ifndef PRINTF_NTOA_BUFFER_SIZE
#define PRINTF_NTOA_BUFFER_SIZE    32U
#endif

///////////////////////////////////////////////////////////////////////////////

// internal flag definitions
#define FLAGS_ZEROPAD   (1U <<  0U)
#define FLAGS_LEFT      (1U <<  1U)
#define FLAGS_PLUS      (1U <<  2U)
#define FLAGS_SPACE     (1U <<  3U)
#define FLAGS_HASH      (1U <<  4U)
#define FLAGS_UPPERCASE (1U <<  5U)
#define FLAGS_CHAR      (1U <<  6U)
#define FLAGS_SHORT     (1U <<  7U)
#define FLAGS_LONG      (1U <<  8U)
#define FLAGS_LONG_LONG (1U <<  9U)
#define FLAGS_PRECISION (1U << 10U)
#define FLAGS_ADAPT_EXP (1U << 11U)

// internal buffer output
static inline void _out_buffer(char character, void* buffer, uint32_t idx, uint32_t maxlen)
{
	if (idx < maxlen) {
		((char*)buffer)[idx] = character;
	}
}

// internal secure strlen
// \return The length of the string (excluding the terminating 0) limited by 'maxsize'
static inline uint32_t _strnlen_s(const char* str, uint32_t maxsize)
{
	const char* s;
	for (s = str; *s && maxsize--; ++s);
	return (unsigned int)(s - str);
}

// internal test if char is a digit (0-9)
// \return true if char is a digit
static inline uint8_t _is_digit(char ch)
{
	return (ch >= '0') && (ch <= '9');
}

// internal ASCII string to unsigned int conversion
static inline uint32_t _atoi(const char** str)
{
	uint32_t i = 0U;
	while (_is_digit(**str)) {
		i = i * 10U + (unsigned int)(*((*str)++) - '0');
	}
	return i;
}

// output the specified string in reverse, taking care of any zero-padding
static uint32_t _out_rev(char* buffer, uint32_t idx, uint32_t maxlen, const char* buf, uint32_t len, unsigned int width, unsigned int flags)
{
	const uint32_t start_idx = idx;

	// pad spaces up to given width
	if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
		for (uint32_t i = len; i < width; i++) {
			_out_buffer(' ', buffer, idx++, maxlen);
		}
	}

	// reverse string
	while (len) {
		_out_buffer(buf[--len], buffer, idx++, maxlen);
	}

	// append pad spaces up to given width
	if (flags & FLAGS_LEFT) {
		while (idx - start_idx < width) {
			_out_buffer(' ', buffer, idx++, maxlen);
		}
	}

	return idx;
}

// internal itoa format
static uint32_t _ntoa_format(char* buffer, uint32_t idx, uint32_t maxlen, char* buf, uint32_t len, bool negative, unsigned int base, unsigned int prec, unsigned int width, unsigned int flags)
{
	// pad leading zeros
	if (!(flags & FLAGS_LEFT)) {
		if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) {
			width--;
		}
		while ((len < prec) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
			buf[len++] = '0';
		}
		while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
			buf[len++] = '0';
		}
	}

	// handle hash
	if (flags & FLAGS_HASH) {
		if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width))) {
			len--;
			if (len && (base == 16U)) {
				len--;
			}
		}
		if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
			buf[len++] = 'x';
		}
		else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
			buf[len++] = 'X';
		}
		else if ((base == 2U) && (len < PRINTF_NTOA_BUFFER_SIZE)) {
			buf[len++] = 'b';
		}
		if (len < PRINTF_NTOA_BUFFER_SIZE) {
			buf[len++] = '0';
		}
	}

	if (len < PRINTF_NTOA_BUFFER_SIZE) {
		if (negative) {
			buf[len++] = '-';
		}
		else if (flags & FLAGS_PLUS) {
			buf[len++] = '+';  // ignore the space if the '+' exists
		}
		else if (flags & FLAGS_SPACE) {
			buf[len++] = ' ';
		}
	}

	return _out_rev(buffer, idx, maxlen, buf, len, width, flags);
}

// internal itoa for 'long' type
static uint32_t _ntoa_long(char* buffer, uint32_t idx, uint32_t maxlen, unsigned long value, bool negative, unsigned long base, unsigned int prec, unsigned int width, unsigned int flags)
{
	char buf[PRINTF_NTOA_BUFFER_SIZE];
	uint32_t len = 0U;

	// no hash for 0 values
	if (!value) {
		flags &= ~FLAGS_HASH;
	}

	// write if precision != 0 and value is != 0
	if (!(flags & FLAGS_PRECISION) || value) {
		do {
			const char digit = (char)(value % base);
			buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
			value /= base;
		} while (value && (len < PRINTF_NTOA_BUFFER_SIZE));
	}

	return _ntoa_format(buffer, idx, maxlen, buf, len, negative, (unsigned int)base, prec, width, flags);
}

// internal vsnprintf
int _vsnprintf(char* buffer, const uint32_t maxlen, const char* format, va_list va)
{
	unsigned int flags, width, precision, n;
	uint32_t idx = 0U;

	if (!buffer)
		return 0;

	while (*format)
	{
		// format specifier?  %[flags][width][.precision][length]
		if (*format != '%') {
			// no
			_out_buffer(*format, buffer, idx++, maxlen);
			format++;
			continue;
		}
		else {
			// yes, evaluate it
			format++;
		}

		// evaluate flags
		flags = 0U;
		do {
			switch (*format) {
				case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
				case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
				case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
				case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
				case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
				default :                                   n = 0U; break;
			}
		} while (n);

		// evaluate width field
		width = 0U;
		if (_is_digit(*format)) {
			width = _atoi(&format);
		}
		else if (*format == '*') {
			const int w = va_arg(va, int);
			if (w < 0) {
				flags |= FLAGS_LEFT;    // reverse padding
				width = (unsigned int)-w;
			}
			else {
				width = (unsigned int)w;
			}
			format++;
		}

		// evaluate precision field
		precision = 0U;
		if (*format == '.') {
			flags |= FLAGS_PRECISION;
			format++;
			if (_is_digit(*format)) {
				precision = _atoi(&format);
			}
			else if (*format == '*') {
				const int prec = (int)va_arg(va, int);
				precision = prec > 0 ? (unsigned int)prec : 0U;
				format++;
			}
		}

		// evaluate length field
		switch (*format) {
			case 'l' :
				flags |= FLAGS_LONG;
				format++;
				if (*format == 'l') {
					flags |= FLAGS_LONG_LONG;
					format++;
				}
				break;
			case 'h' :
				flags |= FLAGS_SHORT;
				format++;
				if (*format == 'h') {
					flags |= FLAGS_CHAR;
					format++;
				}
				break;
#if defined(PRINTF_SUPPORT_PTRDIFF_T)
				case 't' :
				flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
#endif
			case 'j' :
				flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			case 'z' :
				flags |= (sizeof(uint32_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
				format++;
				break;
			default :
				break;
		}

		// evaluate specifier
		switch (*format) {
			case 'd' :
			case 'i' :
			case 'u' :
			case 'x' :
			case 'X' :
			case 'o' :
			case 'b' : {
				// set the base
				unsigned int base;
				if (*format == 'x' || *format == 'X') {
					base = 16U;
				}
				else if (*format == 'o') {
					base =  8U;
				}
				else if (*format == 'b') {
					base =  2U;
				}
				else {
					base = 10U;
					flags &= ~FLAGS_HASH;   // no hash for dec format
				}
				// uppercase
				if (*format == 'X') {
					flags |= FLAGS_UPPERCASE;
				}

				// no plus or space flag for u, x, X, o, b
				if ((*format != 'i') && (*format != 'd')) {
					flags &= ~(FLAGS_PLUS | FLAGS_SPACE);
				}

				// ignore '0' flag when precision is given
				if (flags & FLAGS_PRECISION) {
					flags &= ~FLAGS_ZEROPAD;
				}

				// convert the integer
				if ((*format == 'i') || (*format == 'd')) {
					// signed
					if (flags & FLAGS_LONG) {
						const long value = va_arg(va, long);
						idx = _ntoa_long(buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
					}
					else {
						const int value = (flags & FLAGS_CHAR) ? (char)va_arg(va, int) : (flags & FLAGS_SHORT) ? (short int)va_arg(va, int) : va_arg(va, int);
						idx = _ntoa_long(buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
					}
				}
				else {
					// unsigned
					if (flags & FLAGS_LONG) {
						idx = _ntoa_long(buffer, idx, maxlen, va_arg(va, unsigned long), 0, base, precision, width, flags);
					}
					else {
						const unsigned int value = (flags & FLAGS_CHAR) ? (unsigned char)va_arg(va, unsigned int) : (flags & FLAGS_SHORT) ? (unsigned short int)va_arg(va, unsigned int) : va_arg(va, unsigned int);
						idx = _ntoa_long(buffer, idx, maxlen, value, 0, base, precision, width, flags);
					}
				}
				format++;
				break;
			}
			case 'c' : {
				unsigned int l = 1U;
				// pre padding
				if (!(flags & FLAGS_LEFT)) {
					while (l++ < width) {
						_out_buffer(' ', buffer, idx++, maxlen);
					}
				}
				// char output
				_out_buffer((char)va_arg(va, int), buffer, idx++, maxlen);
				// post padding
				if (flags & FLAGS_LEFT) {
					while (l++ < width) {
						_out_buffer(' ', buffer, idx++, maxlen);
					}
				}
				format++;
				break;
			}

			case 's' : {
				const char* p = va_arg(va, char*);
				uint32_t l = _strnlen_s(p, precision ? precision : (uint32_t)-1);
				// pre padding
				if (flags & FLAGS_PRECISION) {
					l = (l < precision ? l : precision);
				}
				if (!(flags & FLAGS_LEFT)) {
					while (l++ < width) {
						_out_buffer(' ', buffer, idx++, maxlen);
					}
				}
				// string output
				while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--)) {
					_out_buffer(*(p++), buffer, idx++, maxlen);
				}
				// post padding
				if (flags & FLAGS_LEFT) {
					while (l++ < width) {
						_out_buffer(' ', buffer, idx++, maxlen);
					}
				}
				format++;
				break;
			}

			case 'p' : {
				width = sizeof(void*) * 2U;
				flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;

				idx = _ntoa_long(buffer, idx, maxlen, (unsigned long)((uintptr_t)va_arg(va, void*)), 0, 16U, precision, width, flags);

				format++;
				break;
			}

			case '%' :
				_out_buffer('%', buffer, idx++, maxlen);
				format++;
				break;

			default :
				_out_buffer(*format, buffer, idx++, maxlen);
				format++;
				break;
		}
	}

	// termination
	_out_buffer((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);

	// return written chars without terminating \0
	return (int)idx;
}