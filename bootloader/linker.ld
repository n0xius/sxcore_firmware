OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_bldr_start)

MEMORY
{
	FLASH_BOOTLOADER : ORIGIN = 0x8000000, LENGTH = 0x3000
    FLASH_FIRMWARE : ORIGIN = 0x8003000, LENGTH = 0x1CC00
    STACK : ORIGIN = 0x20000000, LENGTH = 0x300
    SPI_BUFFER : ORIGIN = 0x20000800, LENGTH = 0x200
    SRAM : ORIGIN = 0x20002000, LENGTH = 0x2000
}

SECTIONS
{
	/* =========== CODE section =========== */

    . = ALIGN(4);

	PROVIDE(__bootloader = ORIGIN(FLASH_BOOTLOADER));
    PROVIDE(__firmware = ORIGIN(FLASH_FIRMWARE));

	PROVIDE(__start__ = ORIGIN(FLASH_BOOTLOADER));
    . = __start__;
	
    . = ALIGN(4);

	.text   : ALIGN(4)
	{
		*(.text.start)
		*(.text*)
		*(.rodata*)
		. = ALIGN(4);
	} > FLASH_BOOTLOADER

	/* =========== DATA section =========== */

	. = ALIGN(4);

	.data : ALIGN(4)
	{
		__data_start__ = .;
		*(.data*)
		__data_end__ = .;
	} > STACK AT> FLASH_BOOTLOADER
	
	. = ALIGN(4);
    
	.bss : ALIGN(4)
	{
		__bss_start__ = .;
		*(.bss* COMMON)
		__bss_end__ = .;
	} > STACK

    . = ALIGN(4);

	/* =========== MISC =========== */

    __spi_buffer__ = ORIGIN(SPI_BUFFER);

    __stack_top__    = 0x20002000;
    __stack_bottom__ = 0x20000300;
}
