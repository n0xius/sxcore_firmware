#pragma once

#include <gd32f3x0.h>

void handle_usb_transfers();

extern uint32_t usbfs_prescaler;
extern uint32_t* g_ram_function_table;