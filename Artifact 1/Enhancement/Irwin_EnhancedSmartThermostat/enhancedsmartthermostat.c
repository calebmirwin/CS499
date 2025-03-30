/******************************************************************************
 *  FILE: enhancedsmartthermostat.c
 *  AUTHOR: Caleb Irwin
 *  DATE: 03/23/2025
 *
 *  DESCRIPTION:
 *      Embedded firmware for a smart thermostat based on TI SimpleLink RTOS.
 *      Provides real-time temperature monitoring, WiFi connectivity, TCP
 *      communication with remote UI, time sync via NTP, and scheduling features.
 *
 *      Modules:
 *          - I2C temperature sensor read
 *          - Button interrupt and event queue
 *          - WiFi setup and socket server
 *          - Schedule logic and clock synchronization
 ******************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/UART2.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>
#include <ti/drivers/net/wifi/wlan.h>
#include <ti/drivers/net/wifi/netcfg.h>
#include <ti/drivers/net/wifi/sl_socket.h>
#include <ti/drivers/net/wifi/netapp.h>
#include <ti/drivers/net/wifi/device.h>
#include <ti/net/utils/clock_sync.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/BIOS.h>

#include "ti_drivers_config.h"

#define SPAWN_TASK_PRIORITY  2
#define TASK_STACK_SIZE      2048

// Wi-Fi Configuration Constants
#define WIFI_SSID       "SSID NAME"
#define WIFI_PASSWORD   "SSID PASSWORD"
#define WIFI_SECURITY   SL_WLAN_SEC_TYPE_WPA_WPA2

// Server/Socket Communication Constants
#define SERVER_IP       SL_IPV4_VAL(192,168,50,45)
#define SERVER_PORT     5000
#define SOCKET_TIMEOUT  5000

// Time Synchronization Constant
#define NTP_SERVER      "time.google.com"

// Event Bitmask Definitions
#define EVENT_DECREASE_TEMP (1 << 0)
#define EVENT_INCREASE_TEMP (1 << 1)

// Schedule Data Model
#define NUM_SCHEDULE_EVENTS 2

typedef struct {
    int hour;
    int minute;
    int setpoint;
} ScheduleEntry;

ScheduleEntry schedule[NUM_SCHEDULE_EVENTS];

// Periodic Task Timing Constants
const unsigned long periodButtonCheck = 200;
const unsigned long periodTempUpdate = 500;
const unsigned long periodLEDUpdate = 1000;
const unsigned long periodUARTUpdate = 1000;
const unsigned long periodTimeUpdate = 1000;
const unsigned long periodScheduleCheck = 1000;
const unsigned long periodHeartBeat = 1000;

// UART Output Macro
#define DISPLAY(...) do { \
    snprintf(output, sizeof(output), __VA_ARGS__); \
    UART2_write(uart, &output, strlen(output), NULL); \
} while (0)

// Time Conversion Macro
#define MS_TO_TICKS(ms) ((ms) * 1000 / Clock_tickPeriod)

// Thermostat State Variables
int16_t temperature = 20;
int16_t setpoint = 20;
uint8_t heat = 0;
uint32_t seconds = 0;

// Button Event Flags
volatile unsigned char decreaseTempFlag = 0;
volatile unsigned char increaseTempFlag = 0;

// Sensor Definitions and I2C Buffers
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[3] = {
    { 0x41, 0x0001, "006" },
    { 0x48, 0x0000, "11X" },
    { 0x49, 0x0000, "116" },
};

uint8_t txBuffer[1];
uint8_t rxBuffer[2];
char output[64];

// Timekeeping Variable
struct tm netTime;

// RTOS and Peripheral Handles
pthread_t gSpawnThread;
UART2_Handle uart;
I2C_Transaction i2cTransaction;
I2C_Handle i2c;
Event_Handle buttonEvent;

int serverSocket = -1;
int clientSocket = -1;

// External Functions
extern int32_t ti_net_SlNet_initConfig();

// Function Prototypes
void ButtonCheck(UArg a0, UArg a1);
void TempUpdate(UArg a0, UArg a1);
void LEDUpdate(UArg a0, UArg a1);
void UARTUpdate(UArg a0, UArg a1);
void TimeUpdate(UArg a0, UArg a1);
void ScheduleCheck(UArg a0, UArg a1);
void ServerTask(UArg a0, UArg a1);
void initUART(void);
void initI2C(void);
void initWiFi(void);
void syncTimeWithNTP(void);
int16_t readTemp(void);
void gpioButtonFxn0(uint_least8_t index);
void gpioButtonFxn1(uint_least8_t index);

/*
 * ======== createTasks ========
 * Creates all system tasks with designated stack size and priority.
 * Tasks include button checks, temperature polling, LED status,
 * real-time keeping, schedule checking, UART communication, and
 * server communication.
 */
void createTasks(void) {
    Task_Params taskParams;

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1024;
    taskParams.priority = 1;
    Task_create(ButtonCheck, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1024;
    taskParams.priority = 1;
    Task_create(TempUpdate, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1024;
    taskParams.priority = 2;
    Task_create(LEDUpdate, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1024;
    taskParams.priority = 2;
    Task_create(TimeUpdate, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1024;
    taskParams.priority = 2;
    Task_create(ScheduleCheck, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 1024;
    taskParams.priority = 1;
    Task_create(UARTUpdate, &taskParams, NULL);

    Task_Params_init(&taskParams);
    taskParams.stackSize = 2048;
    taskParams.priority = 2;
    Task_create(ServerTask, &taskParams, NULL);

    DISPLAY("Tasks Created Successfully\r\n\n");
}

/*
 * ======== mainThread ========
 * Main system initialization routine.
 * Initializes GPIOs, peripherals, syncs time, sets default schedule,
 * and enters idle loop.
 */

void *mainThread(void *arg0) {
    GPIO_init();

    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_1);

    initUART();
    initI2C();
    initWiFi();
    DISPLAY("GPIO + UART + I2C + WiFi Initialized\r\n");

    buttonEvent = Event_create(NULL, NULL);

    syncTimeWithNTP();

    // Set default schedule entries
    schedule[0].hour = 8;
    schedule[0].minute = 0;
    schedule[0].setpoint = 20;

    schedule[1].hour = 20;
    schedule[1].minute = 0;
    schedule[1].setpoint = 15;

    createTasks();

    while (1) {
        Task_sleep(BIOS_WAIT_FOREVER);
    }
}


/* Task Functions */

/*
 * ======== ButtonCheck ========
 * Event-driven task that handles button press events.
 * Adjusts the thermostat setpoint up or down based on flags.
 */

void ButtonCheck(UArg a0, UArg a1) {
    while (1) {
        uint32_t events = Event_pend(buttonEvent, Event_Id_NONE, EVENT_DECREASE_TEMP | EVENT_INCREASE_TEMP, 0);

        if (events & EVENT_DECREASE_TEMP) {
            decreaseTempFlag = true;
        }
        if (events & EVENT_INCREASE_TEMP) {
            increaseTempFlag = true;
        }

        if (decreaseTempFlag && setpoint > 0) {
            setpoint--;
            decreaseTempFlag = false;
            DISPLAY("Setpoint decreased: %d\r\n", setpoint);
        }
        if (increaseTempFlag && setpoint < 99) {
            setpoint++;
            increaseTempFlag = false;
            DISPLAY("Setpoint increased: %d\r\n", setpoint);
        }

        Task_sleep(MS_TO_TICKS(periodButtonCheck));
    }
}

/*
 * ======== TempUpdate ========
 * Periodically reads temperature from the I2C sensor
 * and updates the global `temperature` variable.
 */

void TempUpdate(UArg a0, UArg a1) {
    while (1) {
        temperature = readTemp();
        Task_sleep(MS_TO_TICKS(periodTempUpdate));
    }
}

/*
 * ======== LEDUpdate ========
 * Controls LED output based on whether heating is active.
 * Turns LED on when heat is needed (temp < setpoint).
 */
void LEDUpdate(UArg a0, UArg a1) {
    while (1) {
        heat = (temperature < setpoint);
        GPIO_write(CONFIG_GPIO_LED_0, heat ? CONFIG_GPIO_LED_ON : CONFIG_GPIO_LED_OFF);
        Task_sleep(MS_TO_TICKS(periodLEDUpdate));
    }
}

/*
 * ======== UARTUpdate ========
 * Periodically sends current thermostat state and timestamp
 * over UART for debugging/logging purposes.
 */

void UARTUpdate(UArg a0, UArg a1) {
    while (1) {
        struct tm *timeInfo = &netTime;

        // If time information is valid, display it
        if (timeInfo) {
            DISPLAY("<%02d,%02d,%d,%02d:%02d:%02d>\r\n",
                    temperature,     // Display temperature
                    setpoint,        // Display setpoint
                    heat,            // Display heating status (0 or 1)
                    timeInfo->tm_hour,  // Hour
                    timeInfo->tm_min,   // Minute
                    timeInfo->tm_sec);  // Second
        } else {
            DISPLAY("Failed to retrieve time information\r\n");
        }

        Task_sleep(MS_TO_TICKS(periodUARTUpdate));
    }
}

/*
 * ======== TimeUpdate ========
 * Increments internal clock (netTime) every second.
 * Acts as a lightweight software clock when NTP sync is unavailable
 * and instead of updating from NTP continually.
 */
void TimeUpdate(UArg a0, UArg a1) {
    while (1) {
        struct tm *timeInfo = &netTime;

        // Tick the internal clock
        timeInfo->tm_sec++;
        if (timeInfo->tm_sec >= 60) {
            timeInfo->tm_sec = 0;
            timeInfo->tm_min++;
            if (timeInfo->tm_min >= 60) {
                timeInfo->tm_min = 0;
                timeInfo->tm_hour++;
                if (timeInfo->tm_hour >= 24) {
                    timeInfo->tm_hour = 0;
                }
            }
        }

        Task_sleep(MS_TO_TICKS(periodTimeUpdate));
    }
}

/*
 * ======== ScheduleCheck ========
 * Checks if current time matches any scheduled entries.
 * If so, applies the scheduled setpoint.
 */
void ScheduleCheck(UArg a0, UArg a1) {
    while (1) {
        for (int i = 0; i < 2; i++) {
            if (netTime.tm_hour == schedule[i].hour &&
                netTime.tm_min == schedule[i].minute &&
                netTime.tm_sec == 0) {  // Optional: match only once per minute
                setpoint = schedule[i].setpoint;
                DISPLAY("Schedule applied at %02d:%02d - Setpoint: %d\r\n",
                        schedule[i].hour, schedule[i].minute, setpoint);
            }
        }

        Task_sleep(MS_TO_TICKS(periodScheduleCheck));
    }
}

/*
 * ======== ServerTask ========
 * TCP server loop handling communication with remote control Python GUI.
 * Sends state updates and responds to SETPOINT, GETSCHEDULE,
 * and SCHEDULE commands. Handles client ACK logic.
 */
void ServerTask(UArg a0, UArg a1) {
    SlSockAddrIn_t localAddr, clientAddr;
    int addrSize = sizeof(SlSockAddrIn_t);
    char buffer[1024];  // Buffer to hold incoming messages
    int recv_size;

    // Create a TCP socket for the server
    serverSocket = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if (serverSocket < 0) {
        DISPLAY("Socket creation failed\r\n");
        return;
    }

    // Set up the server address
    localAddr.sin_family = SL_AF_INET;
    localAddr.sin_port = sl_Htons(SERVER_PORT);
    localAddr.sin_addr.s_addr = 0;

    if (sl_Bind(serverSocket, (SlSockAddr_t *)&localAddr, sizeof(localAddr)) < 0) {
        DISPLAY("Bind failed\r\n");
        sl_Close(serverSocket);
        return;
    }

    if (sl_Listen(serverSocket, 1) < 0) {
        DISPLAY("Listen failed\r\n");
        sl_Close(serverSocket);
        return;
    }

    DISPLAY("Waiting for client connection...\r\n");

    clientSocket = sl_Accept(serverSocket, (SlSockAddr_t *)&clientAddr, (SlSocklen_t*)&addrSize);
    if (clientSocket < 0) {
        DISPLAY("Client accept failed\r\n");
        sl_Close(serverSocket);
        return;
    }

    DISPLAY("Client connected!\r\n");

    while (1) {
        snprintf(buffer, sizeof(buffer), "TEMP:%d,SETPOINT:%d,HEAT:%d\n",
                 temperature, setpoint, heat);
        sl_Send(clientSocket, buffer, strlen(buffer), 0);
        DISPLAY("Sent data: %s\r", buffer);

        recv_size = sl_Recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (recv_size > 0) {
            buffer[recv_size] = '\0';
            char *line = strtok(buffer, "\n");

            while (line != NULL) {
                bool ackNeeded = false;
                DISPLAY("Received message: %s\r\n", line);

                if (strncmp(line, "SETPOINT:", 9) == 0) {
                    int newSetpoint = atoi(&line[9]);
                    if (newSetpoint >= 0 && newSetpoint <= 99) {
                        setpoint = newSetpoint;
                        DISPLAY("Setpoint updated to %d\r\n", setpoint);
                        ackNeeded = true;
                    } else {
                        DISPLAY("Invalid setpoint value: %d\r\n", newSetpoint);
                    }
                }
                else if (strncmp(line, "GETSCHEDULE", 11) == 0) {
                    snprintf(buffer, sizeof(buffer), "SCHEDULE:[%02d:%02d,%d],[%02d:%02d,%d]\n",
                             schedule[0].hour, schedule[0].minute, schedule[0].setpoint,
                             schedule[1].hour, schedule[1].minute, schedule[1].setpoint);
                    sl_Send(clientSocket, buffer, strlen(buffer), 0);
                    DISPLAY("Responded to GETSCHEDULE with current schedule.\r\n");
                }
                else if (strncmp(line, "SCHEDULE:", 9) == 0) {
                    char *data = &line[9];
                    int h1, m1, s1, h2, m2, s2;
                    int parsed = sscanf(data, "[%d:%d,%d],[%d:%d,%d]", &h1, &m1, &s1, &h2, &m2, &s2);

                    if (parsed == 6 &&
                        h1 >= 0 && h1 < 24 && m1 >= 0 && m1 < 60 && s1 >= 0 && s1 <= 99 &&
                        h2 >= 0 && h2 < 24 && m2 >= 0 && m2 < 60 && s2 >= 0 && s2 <= 99) {

                        schedule[0].hour = h1; schedule[0].minute = m1; schedule[0].setpoint = s1;
                        schedule[1].hour = h2; schedule[1].minute = m2; schedule[1].setpoint = s2;

                        DISPLAY("Schedule updated: [%02d:%02d,%d], [%02d:%02d,%d]\r\n",
                                h1, m1, s1, h2, m2, s2);

                        ackNeeded = true;
                    } else {
                        DISPLAY("Invalid schedule format\r\n");
                    }
                }

                if (ackNeeded) {
                    const char *ack_message = "ACK\n";
                    sl_Send(clientSocket, ack_message, strlen(ack_message), 0);
                }

                line = strtok(NULL, "\n");
            }
        }

        Task_sleep(MS_TO_TICKS(periodHeartBeat));
    }

    sl_Close(clientSocket);
    sl_Close(serverSocket);
}


/*
 * ======== gpioButtonFxn0 ========
 * GPIO interrupt callbacks for user button 0 presses.
 * Posts event to event queue for processing in ButtonCheck().
 */
void gpioButtonFxn0(uint_least8_t index) {
    Event_post(buttonEvent, EVENT_DECREASE_TEMP);
}

/*
 * ======== gpioButtonFxn1 ========
 * GPIO interrupt callbacks for user button 1 presses.
 * Posts event to event queue for processing in ButtonCheck().
 */
void gpioButtonFxn1(uint_least8_t index) {
    Event_post(buttonEvent, EVENT_INCREASE_TEMP);
}


/* Other Function Definitions */

/*
 * ======== initUART ========
 * Initializes UART peripheral for debug output using UART2.
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
 * ======== initI2C ========
 * Initializes I2C peripheral and detects compatible temperature sensor
 * from known address list.
 */
void initI2C(void)
{
    int8_t i, found;
    I2C_Params i2cParams;

    DISPLAY("Initializing I2C Driver - ");

    // Initialize the driver
    I2C_init();

    // Configure the driver
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    // Open the driver
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        DISPLAY("I2C Failed\r\n");
        while (1);
    }

    DISPLAY("I2C Passed\r\n");

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

        DISPLAY("Is this %s? ", sensors[i].id);
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            DISPLAY("Found\r\n");
            found = true;
            break;
        }
        DISPLAY("No\r\n");
    }

    if(found)
    {
        DISPLAY("Detected TMP%s I2C address: %x\r\n", sensors[i].id, i2cTransaction.targetAddress);
    }
    else
    {
        DISPLAY("Temperature sensor not found, contact professor\r\n");
    }
}


/*
 * ======== readTemp ========
 * Reads temperature value from sensor over I2C.
 * Converts sensor reading to degrees Celsius and bounds it between 0–99.
 *
 * RETURNS:
 *      int16_t – current temperature in °C (bounded)
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
        DISPLAY("Error reading temperature sensor (%d)\r\n",i2cTransaction.status);
        DISPLAY("Please power cycle your board by unplugging USB and plugging back in.\r\n");
    }

    return temperature;
 }

/*
 * ======== syncTimeWithNTP ========
 * Attempts to synchronize the internal clock using NTP.
 * If sync fails, sets clock to a default fallback time.
 */

void syncTimeWithNTP(void) {
    int status;

    // Sync time using ClockSync from NTP Server
    status = ClockSync_get(&netTime);

    if (status == 0) {
        DISPLAY("Synchronized Time: %s\r", asctime(&netTime));  // Print time as a string
    } else {
        // NTP failed, set netTime to January 1, 2025 at midnight
        netTime.tm_year = 2025 - 1900; // Year (2025)
        netTime.tm_mon = 0;            // January (0)
        netTime.tm_mday = 1;           // 1st day of the month
        netTime.tm_hour = 0;           // 00:00:00 (midnight)
        netTime.tm_min = 0;
        netTime.tm_sec = 0;

        DISPLAY("Error syncing time with NTP server. Status: %d\n\r", status);
    }
}

/*
 * ======== initWiFi ========
 * Initializes WiFi stack and connects to a configured network (SSID/password).
 * Handles SimpleLink task spawn, station mode configuration, and IP acquisition.
 */
void initWiFi(void)
{
    int32_t status;
    SlWlanSecParams_t secParams;
    SlNetCfgIpV4Args_t ipV4;
    _u8 connected = 0;
    _u16 len, option;

    status = ti_net_SlNet_initConfig();
    if (status != 0) {
        DISPLAY("Network initialization failed\r\n");
        return;
    }

    SPI_init();
    DISPLAY("SPI initialized\r\n");

    // Create Spawn Thread for sl_Task
    pthread_attr_t spawnAttrs;
    struct sched_param priParam;

    pthread_attr_init(&spawnAttrs);
    priParam.sched_priority = SPAWN_TASK_PRIORITY;
    pthread_attr_setschedparam(&spawnAttrs, &priParam);
    pthread_attr_setstacksize(&spawnAttrs, TASK_STACK_SIZE);
    pthread_create(&gSpawnThread, &spawnAttrs, sl_Task, NULL);

    // Clear Old Profile
    sl_Start(NULL, NULL, NULL);
    sl_WlanProfileDel(0xFF);
    sl_Stop(0);

    // Set Station Mode
    sl_Start(NULL, NULL, NULL);
    sl_WlanSetMode(ROLE_STA);
    sl_Stop(0);

    // Start SimpleLink & Connect to WiFi
    status = sl_Start(NULL, NULL, NULL);
    if (status < 0) {
        DISPLAY("Failed to start SimpleLink: %d\r\n", status);
        return;
    }

    DISPLAY("SimpleLink started. Connecting to WiFi...\r\n");

    sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(0, 0, 0, 0), NULL, 0);

    secParams.Key = (_i8 *)WIFI_PASSWORD;
    secParams.KeyLen = strlen(WIFI_PASSWORD);
    secParams.Type = WIFI_SECURITY;

    status = sl_WlanConnect((_i8 *)WIFI_SSID, strlen(WIFI_SSID), 0, &secParams, 0);
    if (status < 0) {
        DISPLAY("WiFi connect failed: %d\r\n", status);
        return;
    }

    // Wait for IP acquisition
    while (!connected) {
        len = sizeof(ipV4);
        option = SL_NETCFG_IPV4_STA_ADDR_MODE;

        status = sl_NetCfgGet(SL_NETCFG_IPV4_STA_ADDR_MODE, &option, &len, (_u8 *)&ipV4);
        if (status == 0 && ipV4.Ip != 0) {
            connected = 1;
            DISPLAY("IP Address: %d.%d.%d.%d\r\n",
                (int)SL_IPV4_BYTE(ipV4.Ip, 3),
                (int)SL_IPV4_BYTE(ipV4.Ip, 2),
                (int)SL_IPV4_BYTE(ipV4.Ip, 1),
                (int)SL_IPV4_BYTE(ipV4.Ip, 0));
        } else {
            Task_sleep(100 * 1000 / Clock_tickPeriod); // 100ms
        }
    }
}

/*
 * ======== SimpleLink EventHandlers ========
 * Handles device-level events from the SimpleLink stack.
 */
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent) { }
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent) { }
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent) {
    DISPLAY("General Event - ID: %lu\r\n", (unsigned long)pDevEvent->Id);
}
void SimpleLinkHttpServerCallback(SlNetAppHttpServerEvent_t *pHttpEvent, SlNetAppHttpServerResponse_t *pHttpResponse) { }
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock) { }
void SimpleLinkSocketTriggerEventHandler(SlSockTriggerEvent_t *pTriggerEvent) { }
void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer) { }
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse) { }
void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pHttpEvent, SlNetAppHttpServerResponse_t *pHttpResponse) { }
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent) {
    DISPLAY("General Event - ID: %lu\r\n", (unsigned long)slFatalErrorEvent->Id);
}
