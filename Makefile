BOARD_TAG    = uno
ARDUINO_LIBS = SPI Ethernet

CXXFLAGS += -DSONOS_WRITE_ONLY_MODE

include /usr/share/arduino/Arduino.mk
