#pragma once

#include <gd32f3x0.h>

/* initialization time delay function */
void delay_init(/*uint8_t sysclk*/);

/* delay ms function */
void delay_ms(uint32_t nms);

/* delay us function */
void delay_us(uint32_t nus);