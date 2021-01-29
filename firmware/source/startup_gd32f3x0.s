.section .text.start
.syntax unified
.align 4

.global _start

_start:
	.word	 __stack_top__						// Top of Stack
.extern reset_handler
	.word	 reset_handler						// Reset Handler
	.word	 nmi_handler						// NMI Handler
	.word	 hard_fault_handler					// Hard Fault Handler
	.word	 mem_manage_handler					// MPU Fault Handler
	.word	 bus_fault_handler					// Bus Fault Handler
	.word	 usage_fault_handler				// Usage Fault Handler
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 svc_handler						// SVCall Handler
	.word	 debug_mon_handler					// Debug Monitor Handler
	.word	 0									// Reserved
	.word	 pend_sv_handler					// PendSV Handler
	.word	 systick_handler					// SysTick Handler

	// external interrupt handler
	.word	 wwdgt_irq_handler					// 16:Window Watchdog Timer
	.word	 lvd_irq_handler					// 17:LVD through EXTI Line detect
	.word	 rtc_irq_handler					// 18:RTC through EXTI Line
	.word	 fmc_irq_handler					// 19:FMC
	.word	 rcu_ctc_irq_handler				// 20:RCU and CTC
	.word	 exti0_1_irq_handler				// 21:EXTI Line 0 and EXTI Line 1
	.word	 exti2_3_irq_handler				// 22:EXTI Line 2 and EXTI Line 3
	.word	 exti4_15_irq_handler				// 23:EXTI Line 4 to EXTI Line 15
	.word	 tsi_irq_handler					// 24:TSI
	.word	 dma_channel0_irq_handler			// 25:DMA Channel 0
	.word	 dma_channel1_2_irq_handler			// 26:DMA Channel 1 and DMA Channel 2
	.word	 dma_channel3_4_irq_handler			// 27:DMA Channel 3 and DMA Channel 4
	.word	 adc_cmp_irq_handler				// 28:ADC and Comparator 0-1
	.word	 timer0_brk_up_trg_com_irq_handler	// 29:TIMER0 Break,Update,Trigger and Commutation
	.word	 timer0_channel_irq_handler			// 30:TIMER0 Channel Capture Compare
	.word	 timer1_irq_handler					// 31:TIMER1
	.word	 timer2_irq_handler					// 32:TIMER2
	.word	 timer5_dac_irq_handler				// 33:TIMER5 and DAC
	.word	 0									// Reserved
.extern timer13_irq_handler
	.word	 timer13_irq_handler				// 35:TIMER13
	.word	 timer14_irq_handler				// 36:TIMER14
	.word	 timer15_irq_handler				// 37:TIMER15
	.word	 timer16_irq_handler				// 38:TIMER16
	.word	 i2c0_ev_irq_handler				// 39:I2C0 Event
	.word	 i2c1_ev_irq_handler				// 40:I2C1 Event
	.word	 spi0_irq_handler					// 41:SPI0
	.word	 spi1_irq_handler					// 42:SPI1
	.word	 usart0_irq_handler					// 43:USART0
	.word	 usart1_irq_handler					// 44:USART1
	.word	 0									// Reserved
	.word	 cec_irq_handler					// 46:CEC
	.word	 0									// Reserved
	.word	 i2c0_er_irq_handler				// 48:I2C0 Error
	.word	 0									// Reserved
	.word	 i2c1_er_irq_handler				// 50:I2C1 Error
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 usbfs_wkup_irq_handler		        // 58:USBFS Wakeup
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 dma_channel5_6_irq_handler			// 64:DMA Channel5 and Channel6
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 0									// Reserved
	.word	 __bootloader + 0x14C		        // 83:USBFS

.extern firmware_reset
    .word firmware_reset
.extern handle_firmware_usb_command
    .word handle_firmware_usb_command

.global g_firmware_version
g_firmware_version:
	.byte 3
	.byte 1
	.byte 0
	.byte 0

.extern handle_firmware_spi_command
    .word handle_firmware_spi_command

    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0

.global g_did_initialize_functions
g_did_initialize_functions:
	.long 0xFFFFFFFF

// dummy Exception Handlers
.thumb_func
.weak nmi_handler
nmi_handler:
.thumb_func
.weak hard_fault_handler
hard_fault_handler:
.thumb_func
.weak mem_manage_handler
mem_manage_handler:
.thumb_func
.weak bus_fault_handler
bus_fault_handler:
.thumb_func
.weak usage_fault_handler
usage_fault_handler:
.thumb_func
.weak svc_handler
svc_handler:
.thumb_func
.weak debug_mon_handler
debug_mon_handler:
.thumb_func
.weak pend_sv_handler
pend_sv_handler:
.thumb_func
.weak systick_handler
systick_handler:

// external interrupt handler
.thumb_func
.weak wwdgt_irq_handler
wwdgt_irq_handler:
.thumb_func
.weak lvd_irq_handler
lvd_irq_handler:
.thumb_func
.weak rtc_irq_handler
rtc_irq_handler:
.thumb_func
.weak fmc_irq_handler
fmc_irq_handler:
.thumb_func
.weak rcu_ctc_irq_handler
rcu_ctc_irq_handler:
.thumb_func
.weak exti0_1_irq_handler
exti0_1_irq_handler:
.thumb_func
.weak exti2_3_irq_handler
exti2_3_irq_handler:
.thumb_func
.weak exti4_15_irq_handler
exti4_15_irq_handler:
.thumb_func
.weak tsi_irq_handler
tsi_irq_handler:
.thumb_func
.weak dma_channel0_irq_handler
dma_channel0_irq_handler:
.thumb_func
.weak dma_channel1_2_irq_handler
dma_channel1_2_irq_handler:
.thumb_func
.weak dma_channel3_4_irq_handler
dma_channel3_4_irq_handler:
.thumb_func
.weak adc_cmp_irq_handler
adc_cmp_irq_handler:
.thumb_func
.weak timer0_brk_up_trg_com_irq_handler
timer0_brk_up_trg_com_irq_handler:
.thumb_func
.weak timer0_channel_irq_handler
timer0_channel_irq_handler:
.thumb_func
.weak timer1_irq_handler
timer1_irq_handler:
.thumb_func
.weak timer2_irq_handler
timer2_irq_handler:
.thumb_func
.weak timer5_dac_irq_handler
timer5_dac_irq_handler:
.thumb_func
.weak timer14_irq_handler
timer14_irq_handler:
.thumb_func
.weak timer15_irq_handler
timer15_irq_handler:
.thumb_func
.weak timer16_irq_handler
timer16_irq_handler:
.thumb_func
.weak i2c0_ev_irq_handler
i2c0_ev_irq_handler:
.thumb_func
.weak i2c1_ev_irq_handler
i2c1_ev_irq_handler:
.thumb_func
.weak spi0_irq_handler
spi0_irq_handler:
.thumb_func
.weak spi1_irq_handler
spi1_irq_handler:
.thumb_func
.weak usart0_irq_handler
usart0_irq_handler:
.thumb_func
.weak usart1_irq_handler
usart1_irq_handler:
.thumb_func
.weak cec_irq_handler
cec_irq_handler:
.thumb_func
.weak i2c0_er_irq_handler
i2c0_er_irq_handler:
.thumb_func
.weak i2c1_er_irq_handler
i2c1_er_irq_handler:
.thumb_func
.weak usbfs_wkup_irq_handler
usbfs_wkup_irq_handler:
.thumb_func
.weak dma_channel5_6_irq_handler
dma_channel5_6_irq_handler:
	B	   .
