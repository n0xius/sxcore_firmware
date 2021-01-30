OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
	FLASH_BOOTLOADER : ORIGIN = 0x8000000, LENGTH = 0x3000
	FLASH_FIRMWARE : ORIGIN = 0x8003000, LENGTH = 0x1CC00
	CONFIG : ORIGIN = 0x801FC00, LENGTH = 0x400
	STACK : ORIGIN = 0x20000300, LENGTH = 0x1D00
	SRAM : ORIGIN = 0x20002000, LENGTH = 0x2000
}

SECTIONS
{
	/* =========== CODE section =========== */

    . = ALIGN(4);

    PROVIDE(__bootloader = ORIGIN(FLASH_BOOTLOADER));
    PROVIDE(__firmware = ORIGIN(FLASH_FIRMWARE));
    PROVIDE(__config = ORIGIN(CONFIG));

	PROVIDE(__start__ = ORIGIN(FLASH_FIRMWARE));
    . = __start__;
	
    . = ALIGN(4);

	.text   : ALIGN(4)
	{
		*(.text.start)
		*(.text*)
		*(.rodata*)
	} > FLASH_FIRMWARE

	. = ALIGN(4);

	.data : ALIGN(4)
	{
		__data_start__ = .;
		*(.data*)
		__data_end__ = .;
	} > STACK AT> FLASH_FIRMWARE
	
	. = ALIGN(4);
    
	.bss : ALIGN(4)
	{
		__bss_start__ = .;
		*(.bss* COMMON)
		__bss_end__ = .;
	} > STACK

    . = ALIGN(4);

	/* =========== MISC =========== */

    __stack_top__    = 0x20002000;
    __stack_bottom__ = 0x20000300;

    __bss_size__ = __bss_end__ - __bss_start__;
    __data_size__ = __data_end__ - __data_start__;
}
