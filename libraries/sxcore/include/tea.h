#pragma once

#include <stdint.h>

void tea_get_key_with_offset(uint32_t* _In, uint32_t* _Out, uint32_t _Offset);
void tea_update_custom_mac(uint32_t* _Data, uint32_t* _Key, uint32_t* _Out);
void tea_encrypt(uint32_t key[4], uint32_t* v);
void tea_decrypt(uint32_t key[4], uint32_t* v);