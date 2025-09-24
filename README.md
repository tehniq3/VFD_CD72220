# VFD_CD72220
based on https://github.com/yo3hjv/VFD-Display-Clock and https://yo3hjv.blogspot.com/2020/12/shack-dual-time-clock-with-pos-vfd.html

![real01](https://github.com/tehniq3/VFD_CD72220/blob/main/photos/RTC_clock_DHT22_real01.png)

![real02](https://github.com/tehniq3/VFD_CD72220/blob/main/photos/RTC_clock_DHT22_real02.png)

The VFD module was powered with +24V and GND (consumption near 0,13A).. Arduino board was powered from VFD module via +5V, GND and control the display via Rx (with D2 of Arduino). 
![howtoconnect](https://github.com/tehniq3/VFD_CD72220/blob/main/technical_info/CD7220_VFD_connections.png)

Software (sketch):
 * v.1 - small updates by Nicu FLORICA (niq_ro) in switch routine (added break command after each case)
 * v.2 - added DHT22 sensor and remove UT (universal time) info 
 * v.3 - changed DS1307RTC library with RTClib.h
 * v.4 - added adjusting using 3 buttons, simmilar as for DPD-201: https://github.com/tehniq3/DPD-201/

![RTC_clock](https://github.com/tehniq3/VFD_CD72220/blob/main/technical_info/RTC_clock_schematic.png)

![RTC_clock_DHT](https://github.com/tehniq3/VFD_CD72220/blob/main/technical_info/RTC_clock_DHT_schematic.png)

![RTC_adj_clock_DHT](https://github.com/tehniq3/VFD_CD72220/blob/main/technical_info/RTC_adj_clock_DHT_schematic.png)


