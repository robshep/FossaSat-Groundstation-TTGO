# Fossa Sat Groundstation for the LilyGo TTGO Lora32

Designed for the LilyGo TTGO LoRa32 board (433Mhz) using its OLED screen

http://www.lilygo.cn/prod_view.aspx?TypeId=50003&Id=1130&FId=t3:50003:3

Some tweaks from the orignal groundstation codebase here:

https://github.com/FOSSASystems/FOSSASAT-1/blob/master/Code/GroundStation/GroundStation.ino

## Usage

Needs jumper from IO23 to LoRa1 for DIO1 interrupt pin on SX1278

## Status

* RX only - Does not deal with input commands
* Uses Lora Pin mappings specific to the LoRa32 v2 rev 1.6
* uses IO23 jumper to LoRa1 (for DIO1 interrupt pin on SX1278)
* Reset Lora module using RST pin on setup()
* Output nice things to the OLED

![Gif me a break](https://i.ibb.co/DLVf80w/IMG-1475.jpg)

## Todo

My intention is for this unit to be a daughter-board to a remote Raspberry Pi SatNogs Station so will dump RX payloads over the USB serial to be picked up by another script for forwarding upstream.

Other use-cases welcome

