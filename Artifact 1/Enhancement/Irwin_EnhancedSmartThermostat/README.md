# Enhanced Smart Thermostat

## Summary

This project enhancement implements a smart thermostat using the TI CC3220S Launchpad with TI-RTOS. The original smart thermostat utlized NORTOS, and the migration to TI-RTOS was required to enable real-time timekeeping and networking. The project features temperature monitoring, user defined schedules and setpoints, WiFi connectivity, and real-time feedback via UART for debugging.

## Setup Instructions
### Prerequisites
#### Hardware:
- TI CC3220S Launchpad

#### Software:
- Code Composer Studio (CCS) 12.8.1  or compatible IDE
- TI-RTOS SDK for CC32XX 7.10.00.13
- SysConfig 1.15.0
- Uniflash 8.8.1.4983 (Optional)

## Usage
In order to tun this program, start by cloning this repository (or downloading the program). Then open enhancedsmartthermostat.c and configure the WiFi Configuration Constants. Then save the file, build the program, and flash the compiled binary using CCS or Uniflash. To view UART information open a COM terminal at 115200 baud to view the real-time logs. You can use the left and right buttons on the CC3220S to then raise or lower the setpoint. When the setpoint is higher than the current temperature, the red LED will be illuminated.

To use the python remote control program, first ensure that the Enhanced Smart Thermostat is running and connected to wifi. Open the python file and configure the constant for the thermostat IP. Save the file and then run the python program. Using this program you should be able to increase or decrease the setpoint and set simple daily schedules. The thermostat is currently configured to utilize UCT for scheduling and logs.