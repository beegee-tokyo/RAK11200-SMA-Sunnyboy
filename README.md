# WisBlock RAK11200 - RAK13300 SMA Sunnyboy to IoT    

| <center><img src="./assets/rakstar.jpg" alt="RAKstar" width=50%></center>  | <center><img src="./assets/RAK-Whirls.png" alt="RAKWireless" width=50%></center> | <center><img src="./assets/WisBlock.png" alt="WisBlock" width=50%></center> | <center><img src="./assets/Yin_yang-48x48.png" alt="BeeGee" width=50%></center>  |
| -- | -- | -- | -- |

----

As my solar panels were installed by [PHilERGY](https://www.philergy.com/), the engineers explained to me how to connect the SMA SunnyBoy inverter to my local WiFi network. This enables the inverter to send its production data and status over the internet to SMA's servers.    
Of course this is very comfortable, but for me as an IoT engineer, I wanted to have direct access to the inverter and collect the data by myself. Luckily, SMA has included a web interface in the Sunnyboy and released an API to request data from the inverter directly.
After some research I found the [SMA SunnyBoy Reader](https://github.com/pkoerber/SMA-SunnyBoy-Reader), which is an easy to use interface to talk directly to the SMA SunnyBoy inverter.    

This application reads solar production and total harvested energy from the [SMA Sunnyboy Inverter](https://www.sma.de/en/products/solarinverters/sunny-boy-15-20-25.html) to show the status of the solar panel energy production.
It shares the information over LoRaWAN for cloud based data processing and visualization.


This project is an update to the original [MHC-Sunnyboy-RAK13300](https://github.com/beegee-tokyo/MHC-Sunnyboy-RAK13300). 

### Differences:

The UDP broadcast of the original application was removed as the new local visualization option doesn't require it.

The monthly and yearly production values are stored locally in the FRAM module to make it easier for the visualization platform.

The RTC was added to have the proper times to update and save the daily, monthly and yearly production values.

The original visualization on a Datacake Dashboard [Around my House](https://app.datacake.de/dashboard/d/b6acccc0-2264-42d4-aec9-94148d7eb76f) was replaced with a local Grafana and InfluxDB installation on a Raspberry Pi4. 
Datacake is still my prefered platform, but without a paid plan the data retention time is limited and makes it impossible to get a bigger overview of the solar production over months (or even years).    

Grafana current and daily values
![Grafana current and daily values](./assets/grafana.png)    

Grafana monthly values
![Grafana monthly values](./assets/grafana-monthly.png)    

_**REMARK**_
This project is made with PlatformIO!

----

## Hardware 

The system is build with modules from the [RAKwireless WisBlock](https://docs.rakwireless.com/Product-Categories/WisBlock/) product line. 
- [WisBlock RAK19007](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK19007/Overview/) Base board
- [WisBlock RAK11200](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK11200/Overview/) Core module to connect to the SMA Sunnyboy inverter over WiFi.
- [WisBlock RAK13300](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK13300/Overview) SX1262 LoRa module for LoRaWAN transmissions
- [WisBlock RAK12002](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK12002/Overview/) RTC module
- [WisBlock RAK15005](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK15005/Overview) 128kByte FRAM module

For the enclosure I designed one for 3D printing that is just large enough for the WisBlock and display.

<center><img src="./assets/device.jpg" alt="Device" width=75%></center>

The 3D files for this enclosure are in the [enclosure](./enclosure) folder of this repo.    

----

## Software
The software on the RAK11200 handles the communication over WiFi to read data from the SMA Sunnyboy inverter and sends the data to a LoRaWAN server using the RAK13300 LoRa module. It summarizes the daily production to monthly and the monthly production to yearly and stores them in the FRAM RAK15005. At the end of each month the values of the monthly and yearly production are sent one time. The RTC RAK12002 is used to get the correct time and date to send the data once a day and once a month.     

### IDE, BSP's and libraries:
- [PlatformIO](https://platformio.org/install)
- [Espressif ESP32 BSP](https://docs.platformio.org/en/latest/boards/index.html#espressif-32)
- [Patch to use RAK11200 with PlatformIO](https://github.com/RAKWireless/WisBlock/tree/master/PlatformIO/RAK11200)
- [SX126x-Arduino LoRaWAN library](https://github.com/beegee-tokyo/SX126x-Arduino)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson.git)
- [SMA SunnyBoy Reader](https://github.com/pkoerber/SMA-SunnyBoy-Reader)
- [ESP32 Application Log](https://github.com/beegee-tokyo/ESP32-MyLog)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [Melopero RV3028](https://github.com/melopero/Melopero_RV-3028_Arduino_Library)
- [CayenneLPP](https://github.com/ElectronicCats/CayenneLPP)
- [FRAM_I2C](https://github.com/RobTillaart/FRAM_I2C)

The libraries are installed automatically by PlatformIO.    

----

# Setting up WiFi credentials
The application uses MultiWiFi class to search for up to two WiFi networks and connect to the one with the better signal quality.
The two WiFi networks can be hard-coded in [src/prefs.cpp](./src/prefs.cpp).
But the suggested solution is to setup the WiFi credentials with the [WisBlock Toolbox](https://play.google.com/store/apps/details?id=tk.giesecke.wisblock_toolbox) over BLE

----

# Setting up LoRaWAN credentials
The LoRaWAN credentials are hard-coded in [src/main.h](./src/main.h). But it is suggested to use other methods to change the LoRaWAN credentials. The firmware offers two options, BLE or AT commands over the USB port:

## 1) Setup over BLE
Using the [WisBlock Toolbox](https://play.google.com/store/apps/details?id=tk.giesecke.wisblock_toolbox) you can connect to the WisBlock over BLE and setup all LoRaWAN parameters like
- Region
- OTAA/ABP
- Confirmed/Unconfirmed message
- ...

More details can be found in the [WisBlock Toolbox](https://github.com/beegee-tokyo/WisBlock-Toolbox)

## 2) Setup over USB port
Using the AT command interface the WisBlock can be setup over the USB port.

A detailed manual for the AT commands are in [AT-Commands.md](./AT-Commands.md)

----

# Setting up SMA Sunnyboy IP address and connection credentials
The IP address of the SMA Sunnyboy converter can be set and read with AT command.    

Query SMA IP address
```
AT+SMA=?
```
Set SMA IP address
```
AT+SMA=192.168.64.64
```

# Checking the saved monthly and yearly production
The saved monthly and daily production values can be read or pre-set with AT command.    

Query monthly production    
```
AT+SMON=?
```
Set monthly production in Wh (only integer values possible)    
```
AT+SMON=60402
```

Query yearly production    
```
AT+SYEAR=?
```
Set yearly production in Wh (only integer values possible)    
```
AT+SYEAR=240000
```

# Setting the RTC time and date
The RTC should be setup to the correct date and time to make sure the daily, monthly and yearly production values are calculated correct.

Query RTC date and time    
```
AT+RTC=?
```
Set RTC date and time (single digit, no leading zeros)    
Format year month date hour (24h format) minute
```
AT+RTC=2023:10:8:14:23
```

----

# Special thanks

Special thanks to @h2zero for his outstanding work on the [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) for the ESP32, that uses only a fraction of memory compared with the BLE library coming with ESP32 Arduino BSP.

Special thanks to @pkoerber for his [SMA SunnyBoy Reader](https://github.com/pkoerber/SMA-SunnyBoy-Reader) that made it so simple to communicate with the SMA Sunnyboy Inverters.