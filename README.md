# Web Server Based Electrical Monitoring System
## Overview
Aim of this project is make a power outlet that can measure electricity current and power. The result can be read from the local web server for monitoring electricity usage. The web server can give an alert if power consumption exceeds the predefined limit.
## Tools
1. ESP32 as the microcontroller.
2. ACS712 as the current sensor that can read AC and DC current. For more information see the datasheet: http://www.allegromicro.com/~/media/files/datasheets/acs712-datasheet.ashx
3. Power Outlet. You can use any type of it depending on your country reference.
4. Breadboard and Jumper Cables to connect between ESP32 and ACS712.
## Wiring
![Wiring image](https://github.com/raihannur45/Web-Server-Based-Electrical-Monitoring-System/blob/main/Images/Wiring.png)
## Libraries
- ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer
- AsyncTCP: https://github.com/me-no-dev/AsyncTCP
## Tutorial
1. Upload the code to your ESP32 board.
2. Upload the HTML file. To install ESP32 Filesystem Uploader in Arduino IDE you can read this: https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/
3. Open the serial monitor and click EN button in your ESP32. Wait until the IP address appears and copy it.
4. Open web browser, paste the IP address on the URL box and hit enter.
5. The page is opened. Try to plug some load and see the changes of current power chart, or power consumption.
6. You can fill the Power Consumption Threshold box with any numbers that indicate your power consumption limit.
7. If the power consumption exceeds the predefined limit, the web server will display a message indicating that the power consumption has exceeded the limit.
