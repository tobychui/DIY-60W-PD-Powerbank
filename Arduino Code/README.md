## Battery Monitoring Code for Arduino Nano

This code is design to work with the UARTS HMI display for showing battery real-time information. According to the display design file, the following are the element ids of on screen editable fields. (You can also find it in the code comment section)

```
Loading page
 *  st: Status text

Onscreen elements:
 *  t0: Battery Percentage
 *  t1: Ouput Voltage (5.00V)
 *  t2: Battery Current
 *  t3: OUTPUT / INPUT (OUTPUT)
 *  t4: Voltage Mode (5V Mode)
 *  t5: Power Output (10.0W)
 *  t6: Battery Voltage (14.2V)
 *  t7: label (TYPE C)
 *  t8: label (BATTERY)
 *  
 *  p2: Battery Icon

Battery Icons ID
 *  2 --> 10% left
 *  3 --> 30% left
 *  4 --> 60% left
 *  5 --> 100%
 *  6 --> Charging Icon
```

The following are the pin configurations

```
Type C READ pin = A0
Battery B+ READ pin = A1
Battery Current READ pin = A4

HMI Display Serial Port: D2, D3 (Software Serial RX, TX)
```



As each current reading modules and voltage measurement has offsets and require calibration, you can adjust the current module sensing value in code using the following two variables

```
float currentCalibrationMultiplier = 4.80263; //m
float currentCalibrationOffset = -2.40132; //c
```

Where it follows the ``` y = mx + c``` formula for current estimation.

### Steps to calibrate the current sensor on A4 GPIO

1. Set the battery charging to idle mode. In this mode the current (y) should be 0 and analog read value (x) should be at 2.5v.
2. Plug a 60W charger into the charger connected to a nearly empty battery. In this mode the current (y) should be 3 (i.e. 20V 3A) and the analog read value (x) should be 4 (2.5 + 2.5 * 3/5 = 4 for 5A current measuring module)
3. Solve the two equation two unknown formula
4. Get the value of m and c in y = mx + c as the calibrated offset values



### Steps to calibrate the voltage sensor on A0 and A1 GPIO

Supply the voltage divider with a constant 14.8v and 20v DC with a lab bench power supply. Adjust the variable resistor until the voltage reading on the Arduino Nano match with your power supply.

