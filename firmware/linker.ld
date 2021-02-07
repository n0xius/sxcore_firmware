OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(reset_handler)

MEMORY
{
	FLASH_BOOTLOADER (rx) : ORIGIN = 0x8000000, LENGTH = 0x3000
	FLASH_FIRMWARE (rx) :   ORIGIN = 0x8003000, LENGTH = 0x1CC00
	CONFIG (rx) :           ORIGIN = 0x801FC00, LENGTH = 0x400
	RAM (xrw) :           ORIGIN = 0x20000300, LENGTH = 0x1D00
}

SECTIONS
{
    . = ALIGN(4);

    /* =========== EXPORTS =========== */

    PROVIDE(__bootloader = ORIGIN(FLASH_BOOTLOADER));
    PROVIDE(__firmware = ORIGIN(FLASH_FIRMWARE));
    PROVIDE(__config = ORIGIN(CONFIG));

	PROVIDE(_start = ORIGIN(FLASH_FIRMWARE));

	/* =========== SECTIONS =========== */

    . = _start;
    . = ALIGN(4);

    .isr_vector :
    {
        KEEP(*(.isr_vector))
        *(.isr_vector*)
    }> FLASH_FIRMWARE
	
    . = ALIGN(4);

	.text :
	{
	    KEEP(*(.vars*));
	    *(.vars*)
		*(.text*)
	} > FLASH_FIRMWARE

	. = ALIGN(4);

	.rodata :
    {
        *(.rodata*)
    }> FLASH_FIRMWARE

	. = ALIGN(4);

	.data :
	{
		__data_start__ = .;
		*(.data*)
		__data_end__ = .;
	} > RAM AT> FLASH_FIRMWARE
	
	. = ALIGN(4);
    
	.bss :
	{
		__bss_start__ = .;
        *(.bss*)
        *(COMMON)
		__bss_end__ = .;
	} > RAM

    . = ALIGN(4);

	/* =========== MISC =========== */

    __stack_bottom__ = 0x20000300;
    __stack_top__    = 0x20002000;
}
