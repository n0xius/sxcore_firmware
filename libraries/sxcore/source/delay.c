#include "delay.h"

void SysTick_delay(uint64_t _Delay)
{
	uint64_t delayValue = _Delay;

	for (volatile int currentVal = SysTick->VAL; ; currentVal = SysTick->VAL)
	{
		uint32_t difference = (currentVal - SysTick->VAL) & 0xFFFFFF;

		if (difference >= delayValue)
			break;

		delayValue -= difference;
	}
}

void delay_init(/*uint8_t sysclk*/)
{
	//uint32_t systemClock = rcu_clock_freq_get(CK_SYS);

	// systemClock == 72000000 = CKPLL_DIV1_5
	// systemClock == 96000000 = CKPLL_DIV2

	// Set Reload Register
	SysTick->LOAD = 0xFFFFFF;

	// Load the SysTick Counter Value
	SysTick->VAL = 0;

	// Enable SysTick IRQ and SysTick Timer
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void delay_ms(uint32_t nms)
{
	SysTick_delay(96000ull * nms);
}

void delay_us(uint32_t nus)
{
	SysTick_delay(96ull * nus);
}

/* delay in micro seconds */
void usb_udelay (const uint32_t usec)
{
    delay_us(usec);
}

/* delay in milliseconds */
void usb_mdelay (const uint32_t msec)
{
    delay_ms(msec);
}