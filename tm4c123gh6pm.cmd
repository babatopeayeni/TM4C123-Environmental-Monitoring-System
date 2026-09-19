--retain=g_pfnVectors

MEMORY
{
    FLASH (RX)  : origin = 0x00000000, length = 0x00040000
    SRAM  (RWX) : origin = 0x20000000, length = 0x00008000
}

/*
 * SRAM starts at:
 *
 *     0x20000000
 *
 * SRAM size:
 *
 *     0x00008000 = 32 KB
 *
 * Top of SRAM:
 *
 *     0x20000000 + 0x00008000
 *     = 0x20008000
 *
 * Cortex-M starts the stack near the top of SRAM.
 */
__STACK_TOP = 0x20008000;

SECTIONS
{
    /*
     * Interrupt vector table
     *
     * Must begin at Flash address 0x00000000.
     */
    .intvecs : {} > 0x00000000

    /*
     * Program instructions.
     */
    .text       : > FLASH

    /*
     * Read-only constant data.
     */
    .const      : > FLASH

    /*
     * TI Clang read-only data.
     */
    .rodata     : > FLASH

    /*
     * C initialization information.
     */
    .cinit      : > FLASH

    /*
     * Constructor-related sections.
     */
    .pinit      : > FLASH
    .init_array : > FLASH

    /*
     * Runtime RAM sections.
     */
    .vtable     : > SRAM
    .data       : > SRAM
    .bss        : > SRAM
    .sysmem     : > SRAM
    .stack      : > SRAM
}