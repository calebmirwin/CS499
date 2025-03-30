/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 *  ======== smartthermostat.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/UART2.h>

/* Driver Configuration */
#include "ti_drivers_config.h"

#define DISPLAY(x) UART2_write(uart, &output, x, NULL);

/* Task Structure */
typedef struct task
{
    unsigned long period;
    unsigned long elapsedTime;
    void (*TaskFct)(void);
} task;

#define NUM_TASKS 4

task tasks[NUM_TASKS];

// Task Periods
const unsigned long tasksPeriodGCD = 100;     // 100ms
const unsigned long periodButtonCheck = 200;  // 200ms
const unsigned long periodTempUpdate = 500;   // 500ms
const unsigned long periodLEDUpdate = 1000;   // 1 second
const unsigned long periodUARTUpdate = 1000;  // 1 second

/* Global Variables */
// Program Global Variables
int16_t temperature = 20;
int16_t setpoint = 20;
uint8_t heat = 0;
uint32_t seconds = 0;
volatile unsigned char decreaseTempFlag = 0;
volatile unsigned char increaseTempFlag = 0;
volatile unsigned char TimerFlag = 0;

// UART Global Variables
char output[64];
UART2_Handle uart;

// I2C Global Variables
static const struct
{
    uint8_t address;
    uint8_t resultReg;
    char *id;
}
sensors[3] =
{
    { 0x48, 0x0000, "11X" },
    { 0x49, 0x0000, "116" },
    { 0x41, 0x0001, "006" }
};
uint8_t txBuffer[1];
uint8_t rxBuffer[2];
I2C_Transaction i2cTransaction;
I2C_Handle i2c;

// Timer Global Variables
Timer_Handle timer0;

/* Function Prototypes */
void ButtonCheck(void);
void TempUpdate(void);
void LEDUpdate(void);
void UARTUpdate(void);
void initUART(void);
void initI2C(void);
void initTimer(void);
int16_t readTemp(void);

/* Callback Functions */

/*
 *  ======== timerCallback ========
 *  Callback function triggered by the timer interrupt.
 *  This function is called periodically based on the timer's configuration
 *  (e.g., every 0.1 seconds) and raises TimerFlag each time.
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    TimerFlag = 1;
}

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *  Raises decreaseTempFlag.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    decreaseTempFlag = 1;
}


/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  Raises increaseTempFlag.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    increaseTempFlag = 1;
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    unsigned char i = 0; // Task Index

    // Initialize GPIO
    GPIO_init();
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_1);

    // Initialize Other Peripherals
    initUART();
    initI2C();
    initTimer();
    DISPLAY(snprintf(output, 64, "GPIO + Peripherals Configured\n\r"))

    /* Initialize Tasks */
    // Task 0, ButtonCheck
    tasks[i].period = periodButtonCheck;
    tasks[i].elapsedTime = 0;
    tasks[i].TaskFct = &ButtonCheck;
    i++;

    // Task 1, TempUpdate
    tasks[i].period = periodTempUpdate;
    tasks[i].elapsedTime = 0;
    tasks[i].TaskFct = &TempUpdate;
    i++;

    // Task 2, LEDUpdate
    tasks[i].period = periodLEDUpdate;
    tasks[i].elapsedTime = 0;
    tasks[i].TaskFct = &LEDUpdate;
    i++;

    // Task 3, UARTUpdate
    tasks[i].period = periodUARTUpdate;
    tasks[i].elapsedTime = 0;
    tasks[i].TaskFct = &UARTUpdate;

    DISPLAY(snprintf(output, 64, "Thermostat Initialized\n\r"));

    // Main Loop
    DISPLAY(snprintf(output, 64, "Starting Task Scheduler\n\r"))
    while (1)
    {
        // When Timer Interrupt Is Triggered
        while (!TimerFlag) {}
        TimerFlag = 0; //Reset Timer Flag

        // Check Elapsed Time For Each Task
        for (i = 0; i < NUM_TASKS; i++)
        {
            // If Specified Period Between Tasks Has Been Reached
            if (tasks[i].elapsedTime >= tasks[i].period)
            {
                tasks[i].TaskFct(); // Execute Task
                tasks[i].elapsedTime = 0; // Reset Time Counter For Task
            }
            tasks[i].elapsedTime += tasksPeriodGCD;
        }
    }

    return (NULL);
}


/* Tasks - Function Definitions */

/*
 *  ======== ButtonCheck ========
 *  Checks if buttons have been pressed using flags and adjusts the set-point accordingly.
 *  Will not change temperature if its attempted to be set below 0 or above 99.
 */
void ButtonCheck(void)
{
    if (decreaseTempFlag)
    {
        decreaseTempFlag = 0; // Reset flag
        // Limit set-point to a minimum value of 0
        if (setpoint > 0)
        {
            setpoint--;
        }
    }
    if (increaseTempFlag)
    {
        increaseTempFlag = 0; // Reset flag
        // Limit set-point to a maximum value of 99
        if (setpoint < 99)
        {
            setpoint++;
        }
    }
}

/*
 *  ======== TempUpdate ========
 *  Calls readTemp() and updates the temperature.
 */
void TempUpdate(void)
{
    temperature = readTemp();
}

/*
 *  ======== LEDUpdate ========
 *  Checks if the current temperature is below the set-point and turns the heat (LED) on.
 *  Otherwise, it turns the heat (LED) off.
 */
void LEDUpdate(void)
{
    if (temperature < setpoint) {
        heat = 1;
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
    }
    else
    {
        heat = 0;
        GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    }
}

/*
 *  ======== UARTUpdate ========
 *  Updates the seconds counter (since this should be called every second) and sends
 *  temperature, set-point, heat status, and seconds since reset over UART to simulate
 *  communicating to the server. Format should be <AA,BB,S,CCCC>
 *
 *  AA - Temperature in decimal, 0-99 degrees Celsius
 *  BB - Set-point temperature in decimal, 0-99 degrees Celsius
 *  S - 0 = Heat OFF, 1 = Heat ON
 *  CCCC - Decimal count of seconds since board has been reset
 */
void UARTUpdate(void)
{
    seconds++; // Update Seconds Counter
    DISPLAY(snprintf(output, 64, "<%02d,%02d,%d,%04d>\n\r", temperature, setpoint, heat, seconds));
}


/* Other Function Definitions */

/*
 *  ======== initUART ========
 *  Function to initialize UART peripheral.
 */
void initUART(void)
{
    UART2_Params uartParams;
    size_t bytesRead;
    size_t bytesWritten = 0;
    uint32_t status     = UART2_STATUS_SUCCESS;

    /* Create a UART where the default read and write mode is BLOCKING */
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 115200;

    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    if (uart == NULL)
    {
        /* UART2_open() failed */
        while (1) {}
    }
}

/*
 *  ======== initI2C ========
 *  Function to initialize I2C peripheral.
 */
void initI2C(void)
{
    int8_t i, found;
    I2C_Params i2cParams;

    DISPLAY(snprintf(output, 64, "Initializing I2C Driver - "))

    // Initialize the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY(snprintf(output, 64, "I2C Failed\n\r"))
        while (1);
    }

    DISPLAY(snprintf(output, 32, "I2C Passed\n\r"))

    // Boards were shipped with different sensors.
    // Welcome to the world of embedded systems.
    // Try to determine which sensor we have.
    // Scan through the possible sensor addresses

    /* Common I2C transaction setup */
    i2cTransaction.writeBuf   = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf    = rxBuffer;
    i2cTransaction.readCount  = 0;

    found = false;
    for (i=0; i<3; ++i)
    {
        i2cTransaction.targetAddress = sensors[i].address;
        txBuffer[0] = sensors[i].resultReg;

        DISPLAY(snprintf(output, 64, "Is this %s? ", sensors[i].id))
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            DISPLAY(snprintf(output, 64, "Found\n\r"))
            found = true;
            break;
        }
        DISPLAY(snprintf(output, 64, "No\n\r"))
    }

    if(found)
    {
        DISPLAY(snprintf(output, 64, "Detected TMP%s I2C address: %x\n\r", sensors[i].id, i2cTransaction.targetAddress))
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Temperature sensor not found, contact professor\n\r"))
    }
}

/*
 *  ======== initTimer ========
 *  Function to initialize timer peripheral.
 */
void initTimer(void)
{
    Timer_Params params;

    // Initialize the driver
    Timer_init();

    // Configure the driver
    Timer_Params_init(&params);
    params.period = 100000;
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;

    // Open the driver
    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {
        /* Failed to initialize timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        while (1) {}
    }

    DISPLAY( snprintf(output, 64, "Timer Configured\n\r"))
}

/*
 *  ======== readTemp ========
 *  Function to read temperature data from sensor.
 */
int16_t readTemp(void)
{
    int j;
    int16_t temperature = 0;

    i2cTransaction.readCount = 2;
    if (I2C_transfer(i2c, &i2cTransaction))
    {
        /*
         * Extract degrees C from the received data;
         * see TMP sensor datasheet
         */
        temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
        temperature *= 0.0078125;

        /*
         * If the MSB is set '1', then we have a 2's complement
         * negative value which needs to be sign extended
         */
        if (rxBuffer[0] & 0x80)
        {
            temperature |= 0xF000;
        }

        // Enforce temperature bounds
        if (temperature < 0) {
            temperature = 0;
        }
        else if (temperature > 99)
        {
            temperature = 99;
        }
    }
    else
    {
        DISPLAY(snprintf(output, 64, "Error reading temperature sensor (%d)\n\r",i2cTransaction.status))
        DISPLAY(snprintf(output, 64, "Please power cycle your board by unplugging USB and plugging back in.\n\r"))
    }

    return temperature;
 }
