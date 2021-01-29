#pragma once

#include <stdint.h>
#include <assert.h>

const uint8_t erista_bct[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const uint8_t mariko_bct[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t stage0_payload[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t stage1_payload[] =
{ 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static_assert(sizeof(erista_bct) != 8, "you have to extract the bct for erista or provide your own");
static_assert(sizeof(mariko_bct) != 8, "you have to extract the bct for mariko or provide your own");
static_assert(sizeof(stage0_payload) != 8, "you have to extract the stage 0 payload or provide your own");
static_assert(sizeof(stage1_payload) != 8, "you have to extract the stage 1 payload or provide your own");