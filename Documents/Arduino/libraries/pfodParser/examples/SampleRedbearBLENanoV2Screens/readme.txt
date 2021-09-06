Modifications need to use pfodParser with RedBear's NanoV2

Because there is a name conflict between Arduino's Stream class and mbed's Stream class

In Order to use pfodParser library with NanoV2 you have to replace
the platform.txt file in
.....\packages\RedBear\hardware\nRF52832\0.0.2

with the platform.txt file in this directory

This modified platform.txt adds a __MBED__ define so that pfodStream.h can include the correct stream header
and rename the conflicting class.