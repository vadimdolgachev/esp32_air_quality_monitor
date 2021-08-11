# esp32_air_quality_monitor

## List of sensors:
- DHT11 https://www.electronicoscaldas.com/datasheet/DHT11_Aosong.pdf
- MH-Z14A https://www.winsen-sensor.com/d/files/infrared-gas-sensor/mh-z14a_co2-manual-v1_01.pdf
- MQ7 https://www.sparkfun.com/datasheets/Sensors/Biometric/MQ-7.pdf

## Retrieving parameters 
`curl http://{IP_ADDRESS_DEVICE}`

## Configuration

Create file `.local.ini` in the root directory and paste WiFi parameters:

```
[env]
build_flags = 
    -D SSID_ENV_VAR=\"SSID_NAME\" 
    -D PASSPHRASE_ENV_VAR=\"PASSWORD\"
```
