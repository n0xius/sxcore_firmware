#include "tea.h"

const uint32_t tea_sum = 0xC6EF3720;
const uint32_t tea_delta = 0x9E3779B9;

void tea_get_key_with_offset(uint32_t* _In, uint32_t* _Out, uint32_t _Offset)
{
	_Out[0] = _In[0] + _Offset;
	_Out[1] = _In[1] + _Offset;
	_Out[2] = _In[2] + _Offset;
	_Out[3] = _In[3] + _Offset;
}

void tea_update_custom_mac(uint32_t* _Data, uint32_t* _Key, uint32_t* _Out)
{
	_Out[3] = _Out[3] - ((_Key[1] + (_Data[0] << 4)) ^ (_Key[2] + (_Data[1] >> 5)) ^ (_Out[2] + tea_sum));
	_Out[2] = _Out[2] - ((_Key[3] + (_Data[1] << 4)) ^ (_Key[0] + (_Data[0] >> 5)) ^ (_Out[3] + tea_sum));
	_Out[1] = _Out[1] - ((_Key[2] + (_Data[0] << 4)) ^ (_Key[3] + (_Data[0] >> 5)) ^ (_Out[0] + tea_sum));
	_Out[0] = _Out[0] - ((_Key[0] + (_Data[1] << 4)) ^ (_Key[1] + (_Data[1] >> 5)) ^ (_Out[1] + tea_sum));
}

void tea_encrypt(uint32_t key[4], uint32_t* v)
{
	for (uint32_t i = 0, sum = 0; i < 32; i++)
	{
		sum += tea_delta;
		v[0] += ((v[1] << 4) + key[0]) ^ (v[1] + sum) ^ ((v[1] >> 5) + key[1]);
		v[1] += ((v[0] << 4) + key[2]) ^ (v[0] + sum) ^ ((v[0] >> 5) + key[3]);
	}
}

void tea_decrypt(uint32_t key[4], uint32_t* v)
{
	for (uint32_t i = 0, sum = tea_sum; i < 32; i++)
	{
		v[1] -= ((v[0] << 4) + key[2]) ^ (v[0] + sum) ^ ((v[0] >> 5) + key[3]);
		v[0] -= ((v[1] << 4) + key[0]) ^ (v[1] + sum) ^ ((v[1] >> 5) + key[1]);
		sum -= tea_delta;
	}
}