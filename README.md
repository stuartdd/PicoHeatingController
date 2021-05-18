# PicoHeatingController
Pico Heating controller using arduino C

# Requires:
* Raspberry PI Pico (Or another board with RP2040 SOC). 
* Arduino IDE with RP2040 configured
* WS-5500 Ethernet adaptor (SPI)
* SSD1306 OLED display module (I2C)
* Some buttons and resistors.

# Optional
* Rust compiled toolchain installed to generate time (schedule) test data.
*    ./upload to generate data and upload to the device

# API
## POST 'ip':
Setting the IP address and overriding the DHCP process (automatic IP address allocation). The ip address is stored in the non volatile memory using the key **'ipAddrStore'** and is read during setup.

```
curl --data-binary "@ipAddress.json" -H 'Content-Type: application/json' http://192.168.1.177/ip
```

The file 'ipAddress.json' would typically contain the following valid JSON:

To set a specific IP address. E.g. ```[192,168,1,123]```

To clear the IP address and revert to DHCP ```[0,0,0,0]```.
Only the first value needs to be '0'.

Note: **The numbers in the file are separated by a Comma ',' NOT a Full stop '.'**

If DHCP fails to allocate an IP address then the default IP address is containd in the constant: ```DEFAULT_IP_ADDRESS``` and is currently defined as: ```192.168.1.177```.

