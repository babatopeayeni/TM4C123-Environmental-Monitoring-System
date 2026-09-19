#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* Function prototypes */
void UART0_Init(void);
void UART0_WriteChar(char c);
void UART0_WriteString(const char *string);


/* UART initialization */
void UART0_Init(void)
{
    /*
     * STEP 1: Enable UART0 clock
     * Manual: Ctrl+F "RCGCUART"
     * Register: SYSCTL_RCGCUART_R
     * Address: 0x400FE618
     * Raw: *((volatile unsigned int *)0x400FE618) |= 0x01U;
     * 0x01 = 0000 0001 -> bit 0 = UART0
     */
    SYSCTL_RCGCUART_R = SYSCTL_RCGCUART_R | 0x01U;


    /*
     * STEP 2: Enable GPIO Port A clock
     * Manual: Ctrl+F "RCGCGPIO"
     * Register: SYSCTL_RCGCGPIO_R
     * Address: 0x400FE608
     * Raw: *((volatile unsigned int *)0x400FE608) |= 0x01U;
     * 0x01 = 0000 0001 -> bit 0 = Port A
     */
    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R | 0x01U;


    /*
     * STEP 3: Wait until UART0 is ready
     * Manual: Ctrl+F "PRUART"
     * Register: SYSCTL_PRUART_R
     * Address: 0x400FEA18
     * Raw: *((volatile unsigned int *)0x400FEA18)
     * bit 0: 0 = not ready, 1 = ready
     */
    while ((SYSCTL_PRUART_R & 0x01U) == 0U)
    {
    }


    /*
     * STEP 4: Wait until GPIO Port A is ready
     * Manual: Ctrl+F "PRGPIO"
     * Register: SYSCTL_PRGPIO_R
     * Address: 0x400FEA08
     * Raw: *((volatile unsigned int *)0x400FEA08)
     * bit 0: 0 = not ready, 1 = Port A ready
     */
    while ((SYSCTL_PRGPIO_R & 0x01U) == 0U)
    {
    }


/*
 * STEP 5: Enable alternate function on PA0 and PA1
 * Manual: Ctrl+F "GPIOAFSEL"
 * Register: GPIO_PORTA_AFSEL_R
 * Address: 0x40004420
 * Raw: *((volatile unsigned int *)0x40004420) |= 0x03U;
 * 0x03 = 0000 0011 -> bits 0 and 1 = PA0 and PA1
 */
GPIO_PORTA_AFSEL_R |= 0x03U;


/*
 * STEP 6: Select UART function for PA0 and PA1
 * Manual: Ctrl+F "GPIOPCTL" and "GPIO Pins and Alternate Functions"
 * Register: GPIO_PORTA_PCTL_R
 * Address: 0x4000452C
 *
 * PA0 PMC0 = 0x1 -> U0RX
 * PA1 PMC1 = 0x1 -> U0TX
 *
 * Raw: *((volatile unsigned int *)0x4000452C) =
 *      (*((volatile unsigned int *)0x4000452C) & ~0x000000FFU)
 *      | 0x00000011U;
 */
GPIO_PORTA_PCTL_R =
    (GPIO_PORTA_PCTL_R & ~0x000000FFU) | 0x00000011U;


/*
 * STEP 7: Disable analog mode on PA0 and PA1
 * Manual: Ctrl+F "GPIOAMSEL"
 * Register: GPIO_PORTA_AMSEL_R
 * Address: 0x40004528
 * Raw: *((volatile unsigned int *)0x40004528) &= ~0x03U;
 * Clear bits 0 and 1 -> PA0/PA1 analog disabled
 */
GPIO_PORTA_AMSEL_R &= ~0x03U;


/*
 * STEP 8: Enable digital function on PA0 and PA1
 * Manual: Ctrl+F "GPIODEN"
 * Register: GPIO_PORTA_DEN_R
 * Address: 0x4000451C
 * Raw: *((volatile unsigned int *)0x4000451C) |= 0x03U;
 * 0x03 = 0000 0011 -> PA0 and PA1 digital enabled
 */
GPIO_PORTA_DEN_R |= 0x03U;


//------------------------------------------------------
//configure UART0 for 115200 baud, 8 data bits, no parity, 1 stop bit.
//_______________________________________________________

/*
 * STEP 9: Disable UART0 before configuration
 * Manual: Ctrl+F "UARTCTL"
 * Register: UART0_CTL_R
 * Address: 0x4000C030
 * Raw: *((volatile unsigned int *)0x4000C030) &= ~0x01U;
 * bit 0 = UARTEN: 0 = disabled, 1 = enabled
 */
UART0_CTL_R &= ~0x01U;


/*
 * STEP 10: Set integer part of baud-rate divisor
 * Manual: Ctrl+F "UARTIBRD"
 * Register: UART0_IBRD_R
 * Address: 0x4000C024
 *
 * System clock = 16,000,000 Hz
 * Baud rate    = 115,200
 *
 * BRD = 16,000,000 / (16 * 115,200)
 *     = 8.680555...
 *
 * Integer part = 8
 *
 * Raw: *((volatile unsigned int *)0x4000C024) = 8U;
 */
UART0_IBRD_R = 8U;


/*
 * STEP 11: Set fractional part of baud-rate divisor
 * Manual: Ctrl+F "UARTFBRD"
 * Register: UART0_FBRD_R
 * Address: 0x4000C028
 *
 * Fraction = 0.680555... * 64
 *          = 43.555...
 *          -> 44
 *
 * Raw: *((volatile unsigned int *)0x4000C028) = 44U;
 */
UART0_FBRD_R = 44U;


//--------------------------------------------------------------------
//we finish UART0_Init() by setting the data format and then turning UART0 on.
//____________________________________________________________________

/*
 * STEP 12: Configure UART data format
 * Manual: Ctrl+F "UARTLCRH"
 * Register: UART0_LCRH_R
 * Address: 0x4000C02C
 *
 * WLEN = 11 -> 8 data bits
 * FEN  = 1  -> FIFO enabled
 * STP2 = 0  -> 1 stop bit
 * PEN  = 0  -> no parity
 *
 * 0x60 = 0110 0000
 *
 * Raw: *((volatile unsigned int *)0x4000C02C) = 0x60U;
 */
UART0_LCRH_R = 0x60U;


/*
 * STEP 13: Enable UART0, transmitter, and receiver
 * Manual: Ctrl+F "UARTCTL"
 * Register: UART0_CTL_R
 * Address: 0x4000C030
 *
 * bit 0 = UARTEN -> UART enable
 * bit 8 = TXE    -> transmitter enable
 * bit 9 = RXE    -> receiver enable
 *
 * 0x301 = UARTEN + TXE + RXE
 *
 * Raw: *((volatile unsigned int *)0x4000C030) = 0x301U;
 */
UART0_CTL_R = 0x301U;
}

//==========================================================
// Send one character through UART0
//==========================================================
void UART0_WriteChar(char c)
{
    /*
     * STEP 1: Wait while the transmit FIFO is full
     * Manual: Ctrl+F "UARTFR"
     * Register: UART0_FR_R
     * Address: 0x4000C018
     *
     * bit 5 = TXFF (Transmit FIFO Full)
     * 0 = FIFO has space
     * 1 = FIFO is full
     *
     * 0x20 = 0010 0000 -> bit 5
     *
     * Raw:
     * while ((*((volatile unsigned int *)0x4000C018) & 0x20U) != 0U)
     */
    while ((UART0_FR_R & 0x20U) != 0U)
    {
    }


    /*
     * STEP 2: Put character into UART data register
     * Manual: Ctrl+F "UARTDR"
     * Register: UART0_DR_R
     * Address: 0x4000C000
     *
     * Writing to UARTDR places the character into
     * the transmit FIFO for transmission.
     *
     * Raw:
     * *((volatile unsigned int *)0x4000C000) =
     *     (unsigned int)c;
     */
    UART0_DR_R = (unsigned int)c;
}

//==================================================
 // Send a string through UART0
 //=================================================
 
void UART0_WriteString(const char *string)
{
    /*
     * Send characters one at a time until '\0'
     * is reached.
     *
     * '\0' marks the end of a C string.
     */
    while (*string != '\0')
    {
        UART0_WriteChar(*string);

        string++;
    }
}

/*
 * Send a floating-point number through UART0
 * Example:
 *     25.23 -> "25.23"
 */
void UART0_WriteFloat(float number)
{
    int whole_part;
    int decimal_part;

    /*
     * Handle negative numbers
     */
    if (number < 0.0f)
    {
        UART0_WriteChar('-');
        number = -number;
    }

    /*
     * Round to two decimal places first.
     *
     * Example:
     * 25.23 * 100 = 2523
     */
    decimal_part = (int)((number * 100.0f) + 0.5f);

    /*
     * Separate whole and decimal portions.
     *
     * 2523 / 100 = 25
     * 2523 % 100 = 23
     */
    whole_part = decimal_part / 100;
    decimal_part = decimal_part % 100;

    /*
     * Print whole part.
     */
    if (whole_part >= 100)
    {
        UART0_WriteChar((char)('0' + ((whole_part / 100) % 10)));
    }

    if (whole_part >= 10)
    {
        UART0_WriteChar((char)('0' + ((whole_part / 10) % 10)));
    }

    UART0_WriteChar((char)('0' + (whole_part % 10)));

    /*
     * Print decimal point.
     */
    UART0_WriteChar('.');

    /*
     * Always print two decimal digits.
     */
    UART0_WriteChar((char)('0' + (decimal_part / 10)));
    UART0_WriteChar((char)('0' + (decimal_part % 10)));
}