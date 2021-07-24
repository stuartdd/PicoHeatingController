# PicoHeatingController

Pico Heating controller using arduino C

## Requires

* Raspberry PI Pico (Or another board with RP2040 SOC)
* Arduino IDE with RP2040 configured
* WS-5500 Ethernet adaptor (SPI)
* SSD1306 OLED display module (I2C)
* Some buttons and resistors.

## Optional

* Rust compiled toolchain installed to generate time (schedule) test data.
*./upload to generate data and upload to the device

## API

POST: /ip Override the IP address.

### POST --> 'ip'

Setting the IP address and overriding the DHCP process (automatic IP address allocation). The ip address is stored in the non volatile memory using the key **'ipAddrStore'** and is read during setup.

``` bash
curl --data-binary "@ipAddress.json" -H 'Content-Type: application/json' http://192.168.1.177/ip
```

The file 'ipAddress.json' would typically contain the following valid JSON:

To set a specific IP address. E.g. ```[192,168,1,123]```

To clear the IP address and revert to DHCP ```[0,0,0,0]```.
Only the first value needs to be '0'.

Note: **The numbers in the file are separated by a Comma ',' NOT a Full stop '.'**

If DHCP fails to allocate an IP address then the default IP address is containd in the constant: ```DEFAULT_IP_ADDRESS``` and is currently defined as: ```192.168.1.177```.

The final IP address is displayed on the last line of the Status screen.

### POST --> 'timeData1' and 'timeData2'

This configures the weekly schedule for timers 1 and 2 (HW and CH)

``` bash
curl --data-binary "@timedata_1.json" -H 'Content-Type: application/json' http://192.168.1.177/timedata1
```

``` bash
curl --data-binary "@timedata_2.json" -H 'Content-Type: application/json' http://192.168.1.177/timedata2
```

The file 'timedata_1.json' and 'timedata_2.json' would typically contain the following valid JSON:

``` json
[90,105,1530,1545,2970,2985,4410,4425,5850,5865,7290,7305,8730,8745]
```

There MUST be an even number of values.

Each pair is an OFF/ON time.

Each number is the number of minutes from 00:00:00 Sunday morning.

For example [90, 100] would set the timer ON at 90 minutes in to Sunday morning and OFF 10 minutes later.

For example [1450, 1460] would set the timer ON at 10 minutes in to Monday morning and OFF 10 minutes later.

Note there are 1440 minutes in a day.

Only 56 numbers are read. Which gives 8 values per day or 4 ON OFF pairs per day.

The week is treated as a linier time span (ignoring day boundaries) so it is valid to have more ON OFF values in a day as long as the number of values is not above 56. You could for example have 28 ON OFF thme pairs on Sunday.

The values are sorted after being read to ensure the numbers go forward in time!
