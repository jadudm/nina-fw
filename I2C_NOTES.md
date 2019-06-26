# I2C NOTES
* There are notes made along the way towards implementing an I2C interface for the nina-fw.*

## Defaults

        SPISClass::SPISClass(spi_host_device_t hostDevice, int dmaChannel, int mosiPin, int misoPin, int sclkPin, int csPin, int readyPin) :
        SPISClass SPIS(VSPI_HOST, 1, 14, 23, 18, 5, 33);

DMA channel: Pin 1
MOSI: Pin 14
MISO: Pin 23
SCLK: Pin 18
CS: 5
READY: 33

1. Why is this information coded into the header? Is that because we can
only have one SPI instance?

2. This all seems to be a lot of work to maintain a small amount of firmware.
Further, the Arduino API does not provide I2C client code... as a result, why
retain the structure? A whole new protocol must be developed.

3. SPIS is not an Arduino library? That is, does it get used on other devices?
Or, was it developed just for this purpose? If so, again... what is gained in 
retaining the firmware when the goal is to have an I2C implementation.

## MOSI/MISO DYSLEXIA

On the Huzzah32, the pin order is:

21 - TX - RX - MI - MO

On the AirLift, it is...

NC - RX - TX - MI - MO

OK. MOSI -> MOSI and MISO -> MISO. I was worried for a minute.

### On the Huzzah32

Now, I need these pins to make sense on the Huzzah32.

FUNCTION   AIRLIFT     HUZZAH
           PIN         PIN
MOSI       IO14        IO18
MISO       IO23        IO19
SCLK       IO18        IO5
CS         IO5         IO21$
READY      IO33        IO17$
RESET      ...

**$** *I chose this arbitrarily*

My firmware needs to reflect that table.

Also, I need a reset circuit? Hm.

## First Run

The parameters above seem to work. Running the Python test harness from Adafruit, I get the following at the Python end:ESP32 SPI webclient test
Firmware vers. bytearray(b'1.3.1\x00')
MAC addr: ['0x28', '0x67', '0x26', '0xa4', '0xae', '0x30']
        Berea24                 RSSI: -66
        NETGEAR12               RSSI: -89
        Berea24_Ext             RSSI: -89
Connecting to AP...
Connected to Berea24    RSSI: -67
My IP address is 192.168.1.160
Traceback (most recent call last):
  File "main.py", line 53, in <module>
  File "adafruit_esp32spi/adafruit_esp32spi.py", line 494, in get_host_by_name
RuntimeError: Failed to request hostname

This is good. There is communication between the Feather M4 (running CircuitPython) and the Huzzah32 (running my firmware).

On the fimrware side, I see... a lot of stuff under debug.

.40COMMAND: e011020742657265613234194368657272795374726565744042656c6c7765746865724e59ee0000
I (31297) wifi: state: run -> init (0)
I (31307) wifi: pm stop, total sleep time: 11109845 us / 15550136 us

I (31307) wifi: new:<6,0>, old:<6,0>, ap:<255,255>, sta:<6,0>, prof:1
E (31317) tcpip_adapter: handle_sta_disconnected 197 esp_wifi_internal_reg_rxcb ret=0x3014
I (31337) wifi: flush txq
I (31337) wifi: stop sw txq
I (31337) wifi: lmac stop hw txq
I (31357) wifi: mode : sta (30:ae:a4:26:67:28)
RESPONSE: e091010101ee
.4COMMAND: e02000ee
RESPONSE: e0a0010106ee
.4COMMAND: e02000ee
RESPONSE: e0a0010106ee
.4COMMAND: e02000ee
RESPONSE: e0a0010106ee
.I (33477) wifi: new:<6,0>, old:<1,0>, ap:<255,255>, sta:<6,0>, prof:1
I (33477) wifi: state: init -> auth (b0)
I (33487) wifi: state: auth -> assoc (0)
I (33497) wifi: state: assoc -> run (10)
I (33517) wifi: connected with Berea24, channel 6, bssid = 60:38:e0:bd:15:b9
I (33527) wifi: pm start, type: 1

I (33527) tcpip_adapter: sta ip: 192.168.1.160, mask: 255.255.255.0, gw: 192.168.1.1
4COMMAND: e02000ee
RESPONSE: e0a0010103ee
.4COMMAND: e02000ee
RESPONSE: e0a0010103ee
.8COMMAND: e0230101ffee7265
RESPONSE: e0a3010742657265613234ee
.8COMMAND: e0250101ffee7265
RESPONSE: e0a50104bdffffffee
.8COMMAND: e0210101ffee7265
RESPONSE: e0a10304c0a801a004ffffff0004c0a80101ee
.20COMMAND: e034010c61646166727569742e636f6dee795374
RESPONSE: e0b4010100ee

This looks good to me. Or, it looks like a starting point. I'm going to pause here. This is a good day's work... or, I have other work I have to do...






