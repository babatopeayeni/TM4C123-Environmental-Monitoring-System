#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "timer.h"

void Timer0_Init(void)
{
    /*
     * STEP 1: Enable the clock for Timer0
     *
     * Manual: Ctrl+F "RCGCTIMER"
     *
     * Register: SYSCTL_RCGCTIMER_R
     * Address: 0x400FE604
     *
     * bit 0 = TIMER0
     *
     * 0x01 = 0000 0001
     *
     * Raw:
     * *((volatile unsigned int *)0x400FE604) |= 0x01U;
     */
    SYSCTL_RCGCTIMER_R |= 0x01U;


    /*
     * STEP 2: Wait until Timer0 is ready
     *
     * Manual: Ctrl+F "PRTIMER"
     *
     * Register: SYSCTL_PRTIMER_R
     * Address: 0x400FEA04
     *
     * bit 0:
     * 0 = Timer0 not ready
     * 1 = Timer0 ready
     */
    while ((SYSCTL_PRTIMER_R & 0x01U) == 0U)
    {
    }
    /*
     * STEP 3: Disable Timer0A before configuration
     *
     * Manual: Ctrl+F "GPTMCTL"
     * Register: TIMER0_CTL_R
     * Address: 0x4003000C
     *
     * bit 0 = TAEN
     * 0 = Timer A disabled
     * 1 = Timer A enabled
     *
     * Raw:
     * *((volatile unsigned int *)0x4003000C) &= ~0x01U;
     */
    TIMER0_CTL_R &= ~0x01U;


    /*
     * STEP 4: Select 32-bit timer configuration
     *
     * Manual: Ctrl+F "GPTMCFG"
     * Register: TIMER0_CFG_R
     * Address: 0x40030000
     *
     * 0x00 = 32-bit timer configuration
     *
     * Raw:
     * *((volatile unsigned int *)0x40030000) = 0x00U;
     */
    TIMER0_CFG_R = 0x00U;


    /*
     * STEP 5: Configure Timer0A as periodic timer
     *
     * Manual: Ctrl+F "GPTMTAMR"
     * Register: TIMER0_TAMR_R
     * Address: 0x40030004
     *
     * TAMR bits [1:0] = 10
     *
     * 10 = Periodic Timer mode
     *
     * 0x02 = 0000 0010
     *
     * Raw:
     * *((volatile unsigned int *)0x40030004) = 0x02U;
     */
    TIMER0_TAMR_R = 0x02U;


    /*
     * STEP 6: Load 1-second count
     *
     * Manual: Ctrl+F "GPTMTAILR"
     * Register: TIMER0_TAILR_R
     * Address: 0x40030028
     *
     * System clock = 16 MHz
     *
     * 16,000,000 clock cycles = 1 second
     *
     * Timer counts:
     * 15,999,999 down to 0
     *
     * Raw:
     * *((volatile unsigned int *)0x40030028) = 15999999U;
     */
    TIMER0_TAILR_R = 15999999U;


    /*
     * STEP 7: Clear any old Timer0A timeout flag
     *
     * Manual: Ctrl+F "GPTMICR"
     * Register: TIMER0_ICR_R
     * Address: 0x40030024
     *
     * bit 0 = TATOCINT
     *
     * Write 1 to bit 0 to clear the
     * Timer0A timeout flag.
     *
     * Raw:
     * *((volatile unsigned int *)0x40030024) = 0x01U;
     */
    TIMER0_ICR_R = 0x01U;


    /*
     * STEP 8: Enable Timer0A
     *
     * Manual: Ctrl+F "GPTMCTL"
     * Register: TIMER0_CTL_R
     * Address: 0x4003000C
     *
     * bit 0 = TAEN
     *
     * Raw:
     * *((volatile unsigned int *)0x4003000C) |= 0x01U;
     */
    TIMER0_CTL_R |= 0x01U;
}

void Timer0_WaitOneSecond(void)
{
    /*
     * STEP 1: Wait for Timer0A timeout
     *
     * Manual: Ctrl+F "GPTMRIS"
     * Register: TIMER0_RIS_R
     * Address: 0x4003001C
     *
     * bit 0 = TATORIS
     *
     * 0 = Timer0A has not timed out
     * 1 = Timer0A has timed out
     *
     * Raw:
     * while ((*((volatile unsigned int *)0x4003001C)
     *         & 0x01U) == 0U)
     */
    while ((TIMER0_RIS_R & 0x01U) == 0U)
    {
    }


    /*
     * STEP 2: Clear Timer0A timeout flag
     *
     * Manual: Ctrl+F "GPTMICR"
     * Register: TIMER0_ICR_R
     * Address: 0x40030024
     *
     * bit 0 = TATOCINT
     *
     * Writing 1 clears the timeout flag.
     *
     * Raw:
     * *((volatile unsigned int *)0x40030024) = 0x01U;
     */
    TIMER0_ICR_R = 0x01U;
}