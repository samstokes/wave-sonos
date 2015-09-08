BOARD_TAG    = uno
ARDUINO_LIBS = SPI Ethernet

# flags:
# * DEBUG: verbose serial output
# * DRY_RUN: disable network and Sonos, just LED and serial output
CXXFLAGS +=

include /usr/share/arduino/Arduino.mk
