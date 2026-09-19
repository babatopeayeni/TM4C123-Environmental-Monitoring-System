#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "uart.h"
#include "timer.h"

/* ============================================================
 * AHT20
 * ============================================================ */

#define AHT20_ADDRESS       0x38U

/* I2C MCS commands when WRITING the register */
#define I2C_MCS_RUN         0x01U
#define I2C_MCS_START       0x02U
#define I2C_MCS_STOP        0x04U
#define I2C_MCS_ACK         0x08U

/* I2C MCS status bits when READING the register */
#define I2C_STATUS_BUSY     0x01U
#define I2C_STATUS_ERROR    0x02U

#define I2C_TIMEOUT         1000000U
#define I2C_TIMEOUT_ERROR   0xFFFFFFFFU


/* ============================================================
 * FUNCTION PROTOTYPES
 * ============================================================ */

void I2C0_Init(void);

unsigned int I2C0_WaitForComplete(void);

void DelayMs(unsigned int ms);

unsigned char AHT20_ReadStatus(void);

void AHT20_Init(void);

void AHT20_TriggerMeasurement(void);

unsigned int AHT20_ReadData(unsigned char buffer[6]);

void AHT20_ConvertData(unsigned char buffer[6]);


/* ============================================================
 * GLOBAL VARIABLES
 * ============================================================
  */


unsigned char data[6] = {0U};

volatile unsigned int init_status = 0U;
volatile unsigned int trigger_status = 0U;
volatile unsigned int read_status = 0U;
volatile unsigned int read_stage = 0U;

volatile unsigned int byte_status[6] = {0U};

volatile unsigned int aht20_status = 0U;


/* Actual bytes returned by AHT20 */

volatile unsigned int debug_d0 = 0U;
volatile unsigned int debug_d1 = 0U;
volatile unsigned int debug_d2 = 0U;
volatile unsigned int debug_d3 = 0U;
volatile unsigned int debug_d4 = 0U;
volatile unsigned int debug_d5 = 0U;


/* Converted measurement data */

volatile unsigned int raw_humidity = 0U;
volatile unsigned int raw_temperature = 0U;

volatile float humidity_percent = 0.0F;
volatile float temperature_c = 0.0F;


/* ============================================================
 * MAIN
 * ============================================================ */

int main(void)
{
    unsigned int attempts;
    unsigned int result;

    /* Configure TM4C123 I2C0 */
    I2C0_Init();
    UART0_Init();
    Timer0_Init();

    UART0_WriteString("Hello from TM4C123!\r\n");

    /* --------------------------------------------------------
     * AHT20 requires time after power-up.
     * Datasheet specifies >= 100 ms.
     * -------------------------------------------------------- */

    DelayMs(100U);


    /* --------------------------------------------------------
     * Read AHT20 status.
     *
     * Bit 7 = BUSY
     * Bit 3 = CAL Enable
     *
     * Bit 3:
     * 1 = calibrated
     * 0 = not calibrated
     * -------------------------------------------------------- */

    aht20_status = AHT20_ReadStatus();


    /* --------------------------------------------------------
     * Initialize only if communication succeeded and
     * calibration bit is 0.
     * -------------------------------------------------------- */

    if ((aht20_status != 0xFFU) &&
        ((aht20_status & 0x08U) == 0U))
    {
        AHT20_Init();

        /* Datasheet requires >= 10 ms after initialization */
        DelayMs(20U);
    }


    /* ========================================================
     * MAIN MEASUREMENT LOOP
     * ======================================================== */

    while (1)
    {
        /* ----------------------------------------------------
         * STEP 1
         *
         * Trigger a new measurement:
         *
         * AC 33 00
         * ---------------------------------------------------- */

        AHT20_TriggerMeasurement();


        /* ----------------------------------------------------
         * STEP 2
         *
         * Measurement normally needs about 80 ms.
         * Give it 100 ms.
         * ---------------------------------------------------- */

       Timer0_WaitOneSecond();


        /* ----------------------------------------------------
         * STEP 3
         *
         * Check BUSY status.
         *
         * Do NOT repeatedly read six measurement bytes while
         * waiting for the sensor.
         *
         * Instead read the STATUS byte until BUSY clears.
         * ---------------------------------------------------- */

        attempts = 0U;

        do
        {
            aht20_status = AHT20_ReadStatus();

            /* Communication error */
            if (aht20_status == 0xFFU)
            {
                break;
            }

            /* Bit 7 = 0 means measurement finished */
            if ((aht20_status & 0x80U) == 0U)
            {
                break;
            }

            DelayMs(5U);

            attempts++;

        } while (attempts < 20U);


        /* ----------------------------------------------------
         * STEP 4
         *
         * If sensor is ready, read exactly six bytes.
         * ---------------------------------------------------- */

        if ((aht20_status != 0xFFU) &&
            ((aht20_status & 0x80U) == 0U))
        {
            result = AHT20_ReadData(data);


            /* ------------------------------------------------
             * Only convert if the complete six-byte read
             * succeeded.
             * ------------------------------------------------ */

    if (result == 0U)
        {
            /*
            * Convert the six raw AHT20 bytes
            * into temperature and humidity.
            */
            AHT20_ConvertData(data);

            /*
            * Send the converted measurements
            * to the PC through UART0.
            */
            UART0_WriteString("Temperature: ");
            UART0_WriteFloat(temperature_c);
            UART0_WriteString(" C\r\n");

            UART0_WriteString("Humidity: ");
            UART0_WriteFloat(humidity_percent);
            UART0_WriteString(" %\r\n");

            UART0_WriteString("\r\n");
        }
        }


        /* ----------------------------------------------------
         * Wait one second before next measurement.
         * ---------------------------------------------------- */

        DelayMs(1000U);
    }
}


/* ============================================================
 * DELAY
 *
 * Assumes TM4C123 system clock = 16 MHz.
 * Uses SysTick.
 * ============================================================ */

void DelayMs(unsigned int ms)
{
    unsigned int i;

    for (i = 0U; i < ms; i++)
    {
        /*
         * SysTick CTRL
         * Address = 0xE000E010
         *
         * Disable SysTick.
         */

        *((volatile unsigned int *)0xE000E010U) = 0U;


        /*
         * SysTick RELOAD
         * Address = 0xE000E014
         *
         * 16 MHz:
         *
         * 16,000 cycles = 1 ms
         *
         * RELOAD = 15999
         */

        *((volatile unsigned int *)0xE000E014U) = 15999U;


        /*
         * SysTick CURRENT
         * Address = 0xE000E018
         */

        *((volatile unsigned int *)0xE000E018U) = 0U;


        /*
         * CTRL = 0x05
         *
         * bit 2 = processor clock
         * bit 0 = enable
         */

        *((volatile unsigned int *)0xE000E010U) = 0x05U;


        /*
         * Wait for COUNTFLAG.
         *
         * Bit 16 = 1 when timer reaches zero.
         */

        while ((
            *((volatile unsigned int *)0xE000E010U)
            & 0x00010000U) == 0U)
        {
        }
    }


    /* Disable SysTick */

    *((volatile unsigned int *)0xE000E010U) = 0U;
}


/* ============================================================
 * I2C0 INITIALIZATION
 *
 * PB2 = I2C0SCL
 * PB3 = I2C0SDA
 *
 * I2C speed = 100 kHz
 * System clock = 16 MHz
 * ============================================================ */

void I2C0_Init(void)
{
    /* --------------------------------------------------------
     * Enable I2C0 clock.
     *
     * SYSCTL_RCGCI2C_R
     * Address = 0x400FE620
     *
     * bit 0 = I2C0
     * -------------------------------------------------------- */

    SYSCTL_RCGCI2C_R |= 0x01U;


    /* --------------------------------------------------------
     * Enable GPIO Port B clock.
     *
     * SYSCTL_RCGCGPIO_R
     * Address = 0x400FE608
     *
     * bit 1 = Port B
     * -------------------------------------------------------- */

    SYSCTL_RCGCGPIO_R |= 0x02U;


    /* Wait for I2C0 */

    while ((SYSCTL_PRI2C_R & 0x01U) == 0U)
    {
    }


    /* Wait for Port B */

    while ((SYSCTL_PRGPIO_R & 0x02U) == 0U)
    {
    }


    /* --------------------------------------------------------
     * PB2 and PB3 alternate function.
     * -------------------------------------------------------- */

    GPIO_PORTB_AFSEL_R |= 0x0CU;


    /* --------------------------------------------------------
     * Select I2C function 3.
     *
     * PB2 = I2C0SCL
     * PB3 = I2C0SDA
     * -------------------------------------------------------- */

    GPIO_PORTB_PCTL_R =
        (GPIO_PORTB_PCTL_R & 0xFFFF00FFU)
        | 0x00003300U;


    /* Disable analog */

    GPIO_PORTB_AMSEL_R &= ~0x0CU;


    /* --------------------------------------------------------
     * SDA PB3 must use open drain.
     * -------------------------------------------------------- */

    GPIO_PORTB_ODR_R |= 0x08U;


    /* Enable digital function */

    GPIO_PORTB_DEN_R |= 0x0CU;


    /* --------------------------------------------------------
     * Enable I2C master.
     *
     * I2C0_MCR_R
     * Address = 0x40020020
     *
     * bit 4 = MFE
     * -------------------------------------------------------- */

    I2C0_MCR_R = 0x10U;


    /* --------------------------------------------------------
     * 16 MHz system clock
     * 100 kHz I2C
     *
     * TPR = 7
     * -------------------------------------------------------- */

    I2C0_MTPR_R = 7U;
}


/* ============================================================
 * WAIT FOR I2C TRANSACTION
 * ============================================================ */

unsigned int I2C0_WaitForComplete(void)
{
    unsigned int timeout;

    /*
     * STEP 1:
     * Wait for the I2C controller to actually become BUSY.
     */
    timeout = I2C_TIMEOUT;

    while ((I2C0_MCS_R & I2C_STATUS_BUSY) == 0U)
    {
        timeout--;

        if (timeout == 0U)
        {
            return I2C_TIMEOUT_ERROR;
        }
    }


    /*
     * STEP 2:
     * Now the transaction has started.
     *
     * Wait until BUSY becomes 0,
     * meaning the operation has completed.
     */
    timeout = I2C_TIMEOUT;

    while ((I2C0_MCS_R & I2C_STATUS_BUSY) != 0U)
    {
        timeout--;

        if (timeout == 0U)
        {
            return I2C_TIMEOUT_ERROR;
        }
    }


    /*
     * STEP 3:
     * Return the final I2C controller status.
     */
    return I2C0_MCS_R;
}

/* ============================================================
 * READ AHT20 STATUS BYTE
 * ============================================================ */

unsigned char AHT20_ReadStatus(void)
{
    unsigned int status;

    /*
     * Select AHT20 in READ mode.
     *
     * AHT20 address = 0x38
     * 0x38 << 1 = 0x70
     * READ bit = 1
     * MSA = 0x71
     */

    I2C0_MSA_R =
        (AHT20_ADDRESS << 1) | 0x01U;

    /*
     * Receive one byte:
     *
     * START + RUN + STOP
     */

    I2C0_MCS_R =
        I2C_MCS_START |
        I2C_MCS_RUN |
        I2C_MCS_STOP;

    status = I2C0_WaitForComplete();

    if (status == I2C_TIMEOUT_ERROR)
    {
        return 0xFFU;
    }

    if ((status & I2C_STATUS_ERROR) != 0U)
    {
        return 0xFFU;
    }

    return (unsigned char)(I2C0_MDR_R & 0xFFU);
}


/* ============================================================
 * INITIALIZE AHT20
 *
 * Send:
 *
 * BE 08 00
 * ============================================================ */

void AHT20_Init(void)
{
    unsigned int status;

    init_status = 0U;


    /* Select AHT20 WRITE mode */

    I2C0_MSA_R =
        (AHT20_ADDRESS << 1);


    /* --------------------------------------------------------
     * BYTE 1 = BE
     * START + RUN
     * -------------------------------------------------------- */

    I2C0_MDR_R = 0xBEU;

    I2C0_MCS_R =
        I2C_MCS_START |
        I2C_MCS_RUN;


    status = I2C0_WaitForComplete();


    if ((status == I2C_TIMEOUT_ERROR) ||
        ((status & I2C_STATUS_ERROR) != 0U))
    {
        init_status = status;

        I2C0_MCS_R =
            I2C_MCS_STOP;

        return;
    }


    /* --------------------------------------------------------
     * BYTE 2 = 08
     * RUN
     * -------------------------------------------------------- */

    I2C0_MDR_R = 0x08U;

    I2C0_MCS_R =
        I2C_MCS_RUN;


    status = I2C0_WaitForComplete();


    if ((status == I2C_TIMEOUT_ERROR) ||
        ((status & I2C_STATUS_ERROR) != 0U))
    {
        init_status = status;

        I2C0_MCS_R =
            I2C_MCS_STOP;

        return;
    }


    /* --------------------------------------------------------
     * BYTE 3 = 00
     * RUN + STOP
     * -------------------------------------------------------- */

    I2C0_MDR_R = 0x00U;

    I2C0_MCS_R =
        I2C_MCS_RUN |
        I2C_MCS_STOP;


    status = I2C0_WaitForComplete();

    init_status = status;
}


/* ============================================================
 * TRIGGER AHT20 MEASUREMENT
 *
 * Send:
 *
 * AC 33 00
 * ============================================================ */

void AHT20_TriggerMeasurement(void)
{
    unsigned int status;

    trigger_status = 0U;


    /* Select WRITE */

    I2C0_MSA_R =
        (AHT20_ADDRESS << 1);


    /* --------------------------------------------------------
     * BYTE 1 = AC
     * -------------------------------------------------------- */

    I2C0_MDR_R = 0xACU;

    I2C0_MCS_R =
        I2C_MCS_START |
        I2C_MCS_RUN;


    status = I2C0_WaitForComplete();


    if ((status == I2C_TIMEOUT_ERROR) ||
        ((status & I2C_STATUS_ERROR) != 0U))
    {
        trigger_status = status;

        I2C0_MCS_R =
            I2C_MCS_STOP;

        return;
    }


    /* --------------------------------------------------------
     * BYTE 2 = 33
     * -------------------------------------------------------- */

    I2C0_MDR_R = 0x33U;

    I2C0_MCS_R =
        I2C_MCS_RUN;


    status = I2C0_WaitForComplete();


    if ((status == I2C_TIMEOUT_ERROR) ||
        ((status & I2C_STATUS_ERROR) != 0U))
    {
        trigger_status = status;

        I2C0_MCS_R =
            I2C_MCS_STOP;

        return;
    }


    /* --------------------------------------------------------
     * BYTE 3 = 00
     * RUN + STOP
     * -------------------------------------------------------- */

    I2C0_MDR_R = 0x00U;

    I2C0_MCS_R =
        I2C_MCS_RUN |
        I2C_MCS_STOP;


    status = I2C0_WaitForComplete();

    trigger_status = status;
}


/* ============================================================
 * READ SIX AHT20 MEASUREMENT BYTES
 *
 * Continuous I2C burst:
 *
 * byte 0 = START + RUN + ACK
 *
 * byte 1 = RUN + ACK
 * byte 2 = RUN + ACK
 * byte 3 = RUN + ACK
 * byte 4 = RUN + ACK
 *
 * byte 5 = RUN + STOP
 *
 * ACK is NOT set for the final byte.
 * ============================================================ */

unsigned int AHT20_ReadData(unsigned char buffer[6])
{
    unsigned int status;
    unsigned int i;


    read_stage = 1U;
    read_status = 0U;


    /* Clear old values */

    for (i = 0U; i < 6U; i++)
    {
        buffer[i] = 0U;
        byte_status[i] = 0U;
    }


    /* --------------------------------------------------------
     * Select AHT20 READ mode.
     * -------------------------------------------------------- */

    I2C0_MSA_R =
        (AHT20_ADDRESS << 1) | 0x01U;


    read_stage = 2U;


    /* ========================================================
     * BYTE 0
     *
     * START + RUN + ACK
     *
     * ACK tells AHT20:
     *
     * "I want another byte."
     * ======================================================== */

    I2C0_MCS_R =
        I2C_MCS_START |
        I2C_MCS_RUN |
        I2C_MCS_ACK;


    status = I2C0_WaitForComplete();


    if (status == I2C_TIMEOUT_ERROR)
    {
        read_stage = 90U;

        return 1U;
    }


    byte_status[0] = status;


    if ((status & I2C_STATUS_ERROR) != 0U)
    {
        read_stage = 91U;

        return 2U;
    }


    buffer[0] =
        (unsigned char)(I2C0_MDR_R & 0xFFU);


    read_stage = 10U;


    /* ========================================================
     * BYTES 1 THROUGH 4
     *
     * RUN + ACK
     * ======================================================== */

    for (i = 1U; i < 5U; i++)
    {
        I2C0_MCS_R =
            I2C_MCS_RUN |
            I2C_MCS_ACK;


        status = I2C0_WaitForComplete();


        if (status == I2C_TIMEOUT_ERROR)
        {
            read_stage = 92U;

            return 3U;
        }


        byte_status[i] = status;


        if ((status & I2C_STATUS_ERROR) != 0U)
        {
            read_stage = 93U;

            return 4U;
        }


        buffer[i] =
            (unsigned char)(I2C0_MDR_R & 0xFFU);


        read_stage = 10U + i;
    }


    /* ========================================================
     * BYTE 5 — FINAL BYTE
     *
     * RUN + STOP
     *
     * ACK is NOT set.
     *
     * Therefore TM4C sends NACK after receiving byte 5.
     * ======================================================== */

    I2C0_MCS_R =
        I2C_MCS_RUN |
        I2C_MCS_STOP;


    status = I2C0_WaitForComplete();


    if (status == I2C_TIMEOUT_ERROR)
    {
        read_stage = 94U;

        return 5U;
    }


    byte_status[5] = status;


    if ((status & I2C_STATUS_ERROR) != 0U)
    {
        read_stage = 95U;

        return 6U;
    }


    buffer[5] =
        (unsigned char)(I2C0_MDR_R & 0xFFU);


    /* --------------------------------------------------------
     * Copy actual sensor bytes into debug variables.
     * -------------------------------------------------------- */

    debug_d0 = buffer[0];
    debug_d1 = buffer[1];
    debug_d2 = buffer[2];
    debug_d3 = buffer[3];
    debug_d4 = buffer[4];
    debug_d5 = buffer[5];


    read_status = status;

    read_stage = 100U;


    return 0U;
}


/* ============================================================
 * CONVERT RAW AHT20 DATA
 * ============================================================ */

void AHT20_ConvertData(unsigned char buffer[6])
{
    /* ========================================================
     * HUMIDITY
     *
     * 20-bit humidity:
     *
     * buffer[1]      = bits 19:12
     * buffer[2]      = bits 11:4
     * buffer[3][7:4] = bits 3:0
     * ======================================================== */

    raw_humidity =
        ((unsigned int)buffer[1] << 12)
        |
        ((unsigned int)buffer[2] << 4)
        |
        ((unsigned int)buffer[3] >> 4);


    humidity_percent =
        ((float)raw_humidity * 100.0F)
        / 1048576.0F;


    /* ========================================================
     * TEMPERATURE
     *
     * 20-bit temperature:
     *
     * buffer[3][3:0] = bits 19:16
     * buffer[4]      = bits 15:8
     * buffer[5]      = bits 7:0
     * ======================================================== */

    raw_temperature =
        (((unsigned int)buffer[3] & 0x0FU) << 16)
        |
        ((unsigned int)buffer[4] << 8)
        |
        (unsigned int)buffer[5];


    temperature_c =
        (((float)raw_temperature * 200.0F)
        / 1048576.0F)
        - 50.0F;
}
 