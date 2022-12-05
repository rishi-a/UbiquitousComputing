
Circuit layout:

+5V    to +5V Arduino
Ground to GND Arduino
SCLK   to digital Pin 2 Arduino
SDIO   to digital Pin 3 Arduino
PD     to digital Pin 4 Arduino

This software is highly experimental, running on Linux only!!!
1. Plug in Arduino...
2. ...wait for one minute to initiate!!!
3. Compile and load arduino.ino to your Arduino.
4. Close Arduino IDE
5. Compile commands-host.c with:
gcc commands-host.c -o commands-host -lm
6. start commands-host with:
./commands-host
7. Wait a minute
8. Press Reset button on Arduino board
Bitmaps will be stored in the subdirectory "pictures" of your installation path until you end commands-host with ctrl + C
You can create a video of the single images using ffmpeg:
ffmpeg -framerate 3 -i %0d.bmp -s 1080x1080 -r 25 -b:v 6M mousecam.mp4
