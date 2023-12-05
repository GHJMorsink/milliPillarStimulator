# milliPillar universal stimulator firmware V.2

### Introduction
-------

The [miliPillar platform](https://pubs.acs.org/doi/10.1021/acsbiomaterials.1c01006) contains a stimulator based on the
Arduino Uno development board. The software for that system is at the moment inflexible and static. It can not produce
all types of stimuli as described in Electrical-stimulation-of-excitable-tissue-design-of-efficacious-and-safe-protocols.pdf.

This project contains the source for a more universal firmware on the same hardware, which can change the stimulus waveform
and repetition rate on the fly.

The definitions for the settings are given using the figure below.

![Figure of waveform with parameters](docs/StimulatorPulse.png)

The parameters are as follows:

| Parameter   | Description                                                       |
|-------------|-------------------------------------------------------------------|
| T0          | Time (in ms) before repeated pulses start after the 'RUn' command |
| T1          | Time (in units of 100us) of the positive going pulse                          |
| T2          | Time (in units of 100us) of  the interphase delay ( 0 means 'no interphase delay') |
| T3          | Time (in units of 100us) of the negative going pulse ( 0 means 'no negative pulse') |
| T4          | Time (in ms) to fill up a complete period (the maximum in case of decreasing periods) |
|  |    |
| V1    | Voltage (in units of 0.1Volt / 100 mV) for the positive pulse |
| V2    | Voltage (in units of 0.1Volt) for the negative pulse |
|  |    |
| DT    | Delta time (in ms): amount which will decrease the period time every time DP pulses are emitted (0 means there is no frequency increase)  |
| DP    | Delta pulses: amount of pulses after which the period time will decrease. The period time decreases every time DP pulses are emitted  |
| DM    | Delta max: the maximum number of dreceasing instances after which the period time is reset to T4 and cycling starts again  |
|  |    |
| RPT   | Number of pulses to be given after the 'RUn' command (0 means the pulses go on forever / until the 'OFf' command) |
|  |    |

Note: all parameters are zero or positive integer numbers (No negative numbers).

### Control
-------

The parameter setting for the stimulator is controlled through the serial port on the Arduino Uno taking use of the onboard USB to serial converter.
On that interface a terminal is configured within the firmware. The terminal has a prompt showing `TERM> ` which invites the user towards
entering a command line.

The terminal has several commands to make optimal use of the stimulator. The terminal can of course also be controlled by a PC program
with graphical interface to enhance the user experience. This project does not include such a program.

The commands recognized by the stimulator are:

| Command and params | Description                                                       |
|--------------------|-------------------------------------------------------------------|
| `HE`               | Gives HElp output  |
| `VE`               | Gives VErsion information |
| `RU [<1..4>]`      | Start giving pulses on 1 (A channel), 2, 3 or 4 (D channel). If channel is omitted all channels are started |
| `OF [<1..4>]`      | Stop emitting pulses on all channels (OFf) or a specific one |
| `SS`               | ShowSettings; shows all current parameters |
| `SV <1..4>,<0..50>,<0..50>` | SetVoltages for channel 1 (A), 2 (B), 3 (C) or 4 (D). The second parameter is for V1, the third for V2 |
| `ST <1..4>,<0..65535>,..,<0..65535>` | SetTimes for a channel. The second parameter is T0, third T1, and up to sixth for T4 |
| `SD <1..4>,<0..65535>,<0..65535>,<0..255>` | SetDeltas for channel A, B, C or D. The second parameter is DT, third is DP, and fourth DM |
| `SC <1..4>,<0..65535>` | Set repeat Count. Set the number of pulses on a channel. |
| `WR`               | Write (store) all settings to EEPROM, including the start-flags. On power up these settings are read from EEPROM. |
|  |    | 
 
Notes:
 - All parameters must be given
 - No spaces allowed around the commas
 - Values are given in decimal positive form
 - `<0..65535>` means a value has to be given between (and included) 0 and 65535
 - brackets mean "optional"
 - On the line, after the command, additional comments can be given if separated by a blank (space comma etc). This allows
 for writing a script with comments for your serial terminal program.
 - A command can be edited while entered and will be executed when the 'enter'-key is pressed (sending a CR on the line)
 - Empty commands do nothing, illegal or wrongly composed commands are responded on with a short explanation
 

### Additional
-----------

Use a terminal program at the PC side. For Windows there are 'PuTTY', 'TeraTerm', and many more. 
Set the serial port to '19200 baud, 8 bits, No parity, 1 stopbit'.

Note for PuTTY:
 - Take care to set in menu *Change settings... --> Terminal --> Keyboard --> The Backspace key* to *Control-H*.


  
Additional descriptions can be found in the `docs`
  
The firmware, written in C, can be found in `src`. It is written and compiled within the Microchip Studio 7.0 IDE, for more 
portability and a flexable environment.
The resulting .hex and .elf files (in the `src/Release` map) can be downloaded to the Arduino using the Arduino IDE.
  
In case you want to compile and/or change the firmware yourself, you need for this project Microchip Studio 7.0. 
Downloadable from https://www.microchip.com/en-us/tools-resources/develop/microchip-studio#Downloads

  
### How to upload hex-file to Arduino
----------------------------------

There are several good descriptions on the web:

- https://arduino.stackexchange.com/questions/60599/how-can-i-upload-a-hex-file-to-an-arduino-uno
- https://forum.arduino.cc/t/the-simplest-way-to-upload-a-compiled-hex-file-without-the-ide/401996
- https://www.aranacorp.com/en/generating-and-uploading-hex-files-to-an-arduino/


### Hardware
-----------

The system can run for channels 1,3 and 4 (A, C and D) without any modification. Because the serial port is attached/shared on the
Arduino on port number D0 and D1 (that is port PD0 and PD1 of the ATMega328) they can not be used for controlling H-bridge 'B'.
This firmware switched the control to port PB0 and PB1 of the ATMega328 (that's 'D8' and 'D9' on the Arduino Uno). 
So, to use these ports (and channel 'B'), one has to cut the traces on the extension board to 'D0' and 'D1' and solder two fine enamelled copper wires from 'D8'
to pin3 of H-BRIDGE1 and from 'D9' to pin4 of H-BRIDGE1.

Furthermore: There is in the original setup no indication pulses are emitted. The new firmware can indicate pulses are given. For doing
that, a LED has to be connected on the extension board. Take a green, yellow, orange or red LED (not blue or white) and a resistor 470 ohm 0.25W.
Connect anode to +5Volt, cathode to resistor, other side of resistor to PC5 / 'SCL' on the Arduino. It will flash on every pulse given by one of the channels.

