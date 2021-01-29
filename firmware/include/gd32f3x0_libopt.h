#pragma once

#include "gd32f3x0_adc.h"
//#include "gd32f3x0_crc.h"
//#include "gd32f3x0_ctc.h"
//#include "gd32f3x0_dbg.h"
#include "gd32f3x0_dma.h"
//#include "gd32f3x0_exti.h"
#include "gd32f3x0_fmc.h"
#include "gd32f3x0_gpio.h"
#include "gd32f3x0_syscfg.h"
#include "gd32f3x0_i2c.h"
//#include "gd32f3x0_fwdgt.h"
#include "gd32f3x0_pmu.h"
#include "gd32f3x0_rcu.h"
//#include "gd32f3x0_rtc.h"
#include "gd32f3x0_spi.h"
#include "gd32f3x0_timer.h"
#include "gd32f3x0_usart.h"
//#include "gd32f3x0_wwdgt.h"
#include "gd32f3x0_misc.h"
//#include "gd32f3x0_tsi.h"

#ifdef GD32F350
//#include "gd32f3x0_cec.h"
#include "gd32f3x0_cmp.h"
//#include "gd32f3x0_dac.h"
#endif /* GD32F350 */

#define SYSMEM_BASE ((uint32_t)0x1FFFEC00)
