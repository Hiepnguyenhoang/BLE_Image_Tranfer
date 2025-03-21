## 1. Introduction

This is a C code BLE interface has been developed for Raspberry Pi (but has also had some testing on Ubuntu, and should work on other Linux systems) 

A Pi running this interface can connect simultaneously to multiple LE devices.

## 2. Compile & Run code server

# Complile

```sh
gcc btferret.c btlib.c -o btferret\
# Run Code

This code must be run with root permissions from the root account or via sudo as follows:
```sh
sudo ./btferret

device.txt

DEVICE = Device name of your choice (don't use any of the key words
                  such as TYPE/NODE/ADDRESS.. or chaos will follow)

TYPE   = MESH     all Pis running btferret/btlib which can then
                  act as CLASSIC/NODE/LE servers and clients
         CLASSIC  classic servers such as Windows/Android/HC05
         LE       LE servers

NODE = 4          Node number in decimal - you choose this

ADDRESS = 00:1E:C0:2D:17:7C   6-byte Bluetooth address
  or
ADDRESS = MATCH_NAME          To find address during a scan by matching name -
                                 CLASSIC and LE type only (see further discussion below)
RANDOM = UNCHANGED            For LE random address that does not change

CHANNEL = 4                   RFCOMM channel for CLASSIC servers (optional)
PIN = 1234                    PIN code for CLASSIC servers (optional)

Đối với LE device có thể 

PRIMARY_SERVICE = 11223344-5566-7788-99AA-BBCCDDEEFF00 //Service
LECHAR = Characteristic name of your choice            //Characteristic
HANDLE = 000B  2-byte handle in hex
PERMIT = 06    Permissions in hex                      //permissions
SIZE = 1       Number of bytes in decimal. Range = 1 to 244 //Size truyền byte
UUID = 2A00    2-byte UUID
UUID = 11223344-5566-7788-99AA-BBCCDDEEFF00    16-byte UUID

//PERMIT 
02 = r   Read only
04 = w   Write no acknowledge
08 = wa  Write with acknowledge
06 = rw  Read/Write no ack
0A = rwa Read/Write ack
10 = n   Notify capable
16 = rwn Read/Write no ack/Notify capable
20 = i   Indicate capable
40 =     Authentication (but see le_pair() for
                         btferret's implementation)
