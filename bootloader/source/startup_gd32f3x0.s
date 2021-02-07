.syntax unified
.cpu cortex-m4
.thumb

/* =========== EXTERNAL FUNCTIONS =========== */
.global _start

/* =========== VECTOR TABLE =========== */

.section	.isr_vector,"a",%progbits
.type	    vector_function_table, %object
.size	    vector_function_table, .-vector_function_table
.global	    vector_function_table

    vector_function_table:
    	.word	 __stack_top__						// top of stack
    	.word	 reset_handler						// vector index 1
    	.word	 nmi_handler						// vector index 2
    	.word	 hard_fault_handler					// vector index 3
    	.word	 mem_manage_handler					// vector index 4
    	.word	 bus_fault_handler					// vector index 5
    	.word	 usage_fault_handler				// vector index 6
    	.word	 0, 0, 0, 0							// vector index 7-10
    	.word	 svc_handler						// vector index 11
    	.word	 debug_mon_handler					// vector index 12
    	.word	 0									// vector index 13
    	.word	 pend_sv_handler					// vector index 14
    	.word	 systick_handler					// vector index 15
    	.word	 wwdgt_irq_handler					// vector index 16
    	.word	 lvd_irq_handler					// vector index 17
    	.word	 rtc_irq_handler					// vector index 18
    	.word	 fmc_irq_handler					// vector index 19
    	.word	 rcu_ctc_irq_handler				// vector index 20
    	.word	 exti0_1_irq_handler				// vector index 21
    	.word	 exti2_3_irq_handler				// vector index 22
    	.word	 exti4_15_irq_handler				// vector index 23
    	.word	 tsi_irq_handler					// vector index 24
    	.word	 dma_channel0_irq_handler			// vector index 25
    	.word	 dma_channel1_2_irq_handler			// vector index 26
    	.word	 dma_channel3_4_irq_handler			// vector index 27
    	.word	 adc_cmp_irq_handler				// vector index 28
    	.word	 timer0_brk_up_trg_com_irq_handler	// vector index 29
    	.word	 timer0_channel_irq_handler			// vector index 30
    	.word	 timer1_irq_handler					// vector index 31
    	.word	 timer2_irq_handler					// vector index 32
    	.word	 timer5_dac_irq_handler				// vector index 33
    	.word	 0									// vector index 34
    	.word	 timer13_irq_handler				// vector index 35
    	.word	 timer14_irq_handler				// vector index 36
    	.word	 timer15_irq_handler				// vector index 37
    	.word	 timer16_irq_handler				// vector index 38
    	.word	 i2c0_ev_irq_handler				// vector index 39
    	.word	 i2c1_ev_irq_handler				// vector index 40
    	.word	 spi0_irq_handler					// vector index 41
    	.word	 spi1_irq_handler					// vector index 42
    	.word	 usart0_irq_handler					// vector index 43
    	.word	 usart1_irq_handler					// vector index 44
    	.word	 0									// vector index 45
    	.word	 cec_irq_handler					// vector index 46
    	.word	 0									// vector index 47
    	.word	 i2c0_er_irq_handler				// vector index 48
    	.word	 0									// vector index 49
    	.word	 i2c1_er_irq_handler				// vector index 50
    	.word	 0, 0, 0, 0, 0, 0, 0				// vector index 51-57
    	.word	 usbfs_wkup_irq_handler		        // vector index 58
    	.word	 0, 0, 0, 0, 0						// vector index 59-63
    	.word	 dma_channel5_6_irq_handler			// vector index 64
    	.word	 0, 0, 0, 0, 0, 0, 0, 0				// vector index 65-72
    	.word	 0, 0, 0, 0, 0, 0, 0, 0				// vector index 73-81
    	.word	 0, 0								// vector index 82
    	.word	 usbfs_irq_handler		            // vector index 83


/* =========== STATIC VARIABLES =========== */

.section	.vars.serial,"w",%progbits
.type	    serial_number, %object
.size	    serial_number, .-serial_number
.global     serial_number

    serial_number:
	    .byte	 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10


.section	.vars.version,"w",%progbits
.type	    bootloader_version, %object
.size	    bootloader_version, .-bootloader_version
.global     bootloader_version

    bootloader_version:
	    .byte	 0xFF, 0xFF, 0xFF, 0xFF


.section	.vars.spi_cmd,"w",%progbits
.type	    handle_spi_command, %object
.size	    handle_spi_command, .-handle_spi_command
.global     handle_spi_command

    handle_spi_command:
        .word handle_bootloader_spi_commands


.section	.vars.version,"w",%progbits
.type	    debug_mode, %object
.size	    debug_mode, .-debug_mode
.global     debug_mode

    debug_mode:
        .byte 0xFF, 0xFF, 0xFF, 0xFF


/* =========== FUNCTIONS =========== */

.section	.text.reset_handler,"ax",%progbits
.size	    reset_handler, .-reset_handler
.global     reset_handler
.thumb_func

    reset_handler:
    /* Call the clock system intitialization function.*/
        ldr     r0, = SystemInit
        blx     r0

    /* Call the application's static data initialization function.*/
        ldr     r0, = initialize_static_data
        blx     r0

    /* Call the application's entry point.*/
        ldr     r0, = main
        bx      r0

.section	.text.data_init,"ax",%progbits
.size	    initialize_static_data, .-initialize_static_data
.global     initialize_static_data
.thumb_func

    initialize_static_data:
    /* copy data (rx) segment into ram (rwx). */
        ldr     r0, = __data_start__
        ldr     r1, = __data_end__
        ldr     r2, = __bss_start__
        b       copy_data_init_loop

    store_data:
        ldr     r4, [r2], #4
        str     r4, [r0], #4

    copy_data_init_loop:
        cmp     r4, r1
        bcc     store_data

    /* zero fill the bss segment. */
        eor     r0, r0
        ldr     r1, = __bss_end__
        b       zero_bss_loop

    zero_bss:
        str     r0, [r2], #4

    zero_bss_loop:
        cmp     r1, r2
        bcc     zero_bss

        bx      lr

.section	.text.default_handler,"ax",%progbits
.size	    default_handler, .-default_handler
.thumb_func

    default_handler:
        b       .

/* =========== ARM CORTEX M4 IRQ's =========== */

.weak	    nmi_handler
.thumb_set  nmi_handler, default_handler

.weak       hard_fault_handler
.thumb_set  hard_fault_handler, default_handler

.weak       mem_manage_handler
.thumb_set  mem_manage_handler, default_handler

.weak       bus_fault_handler
.thumb_set  bus_fault_handler, default_handler

.weak       usage_fault_handler
.thumb_set  usage_fault_handler, default_handler

.weak       svc_handler
.thumb_set  svc_handler, default_handler

.weak       debug_mon_handler
.thumb_set  debug_mon_handler, default_handler

.weak       pend_sv_handler
.thumb_set  pend_sv_handler, default_handler

.weak       systick_handler
.thumb_set  systick_handler, default_handler


/* =========== mcu irq's =========== */

.weak       wwdgt_irq_handler
.thumb_set  wwdgt_irq_handler, default_handler

.weak       lvd_irq_handler
.thumb_set  lvd_irq_handler, default_handler

.weak       rtc_irq_handler
.thumb_set  rtc_irq_handler, default_handler

.weak       fmc_irq_handler
.thumb_set  fmc_irq_handler, default_handler

.weak       rcu_ctc_irq_handler
.thumb_set  rcu_ctc_irq_handler, default_handler

.weak       exti0_1_irq_handler
.thumb_set  exti0_1_irq_handler, default_handler

.weak       exti2_3_irq_handler
.thumb_set  exti2_3_irq_handler, default_handler

.weak       exti4_15_irq_handler
.thumb_set  exti4_15_irq_handler, default_handler

.weak       tsi_irq_handler
.thumb_set  tsi_irq_handler, default_handler

.weak       dma_channel0_irq_handler
.thumb_set  dma_channel0_irq_handler, default_handler

.weak       dma_channel1_2_irq_handler
.thumb_set  dma_channel1_2_irq_handler, default_handler

.weak       dma_channel3_4_irq_handler
.thumb_set  dma_channel3_4_irq_handler, default_handler

.weak       adc_cmp_irq_handler
.thumb_set  adc_cmp_irq_handler, default_handler

.weak       timer0_brk_up_trg_com_irq_handler
.thumb_set  timer0_brk_up_trg_com_irq_handler, default_handler

.weak       timer0_channel_irq_handler
.thumb_set  timer0_channel_irq_handler, default_handler

.weak       timer1_irq_handler
.thumb_set  timer1_irq_handler, default_handler

.weak       timer2_irq_handler
.thumb_set  timer2_irq_handler, default_handler

.weak       timer5_dac_irq_handler
.thumb_set  timer5_dac_irq_handler, default_handler

.weak       timer13_irq_handler
.thumb_set  timer13_irq_handler, default_handler

.weak       timer14_irq_handler
.thumb_set  timer14_irq_handler, default_handler

.weak       timer15_irq_handler
.thumb_set  timer15_irq_handler, default_handler

.weak       timer16_irq_handler
.thumb_set  timer16_irq_handler, default_handler

.weak       i2c0_ev_irq_handler
.thumb_set  i2c0_ev_irq_handler, default_handler

.weak       i2c1_ev_irq_handler
.thumb_set  i2c1_ev_irq_handler, default_handler

.weak       spi0_irq_handler
.thumb_set  spi0_irq_handler, default_handler

.weak       spi1_irq_handler
.thumb_set  spi1_irq_handler, default_handler

.weak       usart0_irq_handler
.thumb_set  usart0_irq_handler, default_handler

.weak       usart1_irq_handler
.thumb_set  usart1_irq_handler, default_handler

.weak       cec_irq_handler
.thumb_set  cec_irq_handler, default_handler

.weak       i2c0_er_irq_handler
.thumb_set  i2c0_er_irq_handler, default_handler

.weak       i2c1_er_irq_handler
.thumb_set  i2c1_er_irq_handler, default_handler

.weak       usbfs_wkup_irq_handler
.thumb_set  usbfs_wkup_irq_handler, default_handler

.weak       dma_channel5_6_irq_handler
.thumb_set  dma_channel5_6_irq_handler, default_handler
