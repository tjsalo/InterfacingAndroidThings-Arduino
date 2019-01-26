The following files comprise the Android Things portion of the software for 
this project.  All of this code was written by the author, except as noted.  
Some snippets of code were copied more or less directly from the Android API 
documentation.

This code can be found in: 
ArduinoIntegration\app\src\main\java\com\salo\android\arduinointegration

MainActivity.java

MainActivity.java principally starts a Thread for each interface, and creates 
HTML pages that contain the temperature data received from the Arduino.  
Additionally, this class writes some logging data to a display, if one is 
attached to the Raspberry Pi.  The NanoHttpd software provides the web 
server; this software only needs to start the NanoHttpd server and create 
HTML web pages in response to HTTP GET commands.

AsyncHandlerThread.java

AsyncHandlerThread.java extends HandlerThread and implements full duplex 
communications over an asynchronous interface.  It uses the Android Things 
UART API.

I2cHandlerThread.java

I2cHandlerThread extends HandlerThread and implements half-duplex 
communications over an I2C interface.  It uses the Android Things I2C API.

SPIThread.java

SPIThread will someday support full duplex communications over an SPI 
interface.

