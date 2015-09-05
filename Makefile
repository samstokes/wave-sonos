BOARD_TAG    = uno
ARDUINO_LIBS = SPI Ethernet

# flags:
# * DEBUG: verbose serial output
CXXFLAGS += -DSONOS_WRITE_ONLY_MODE

include /usr/share/arduino/Arduino.mk
