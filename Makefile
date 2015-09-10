BOARD_TAG    = uno
ARDUINO_LIBS = SPI Ethernet

# Required flags (configure these according to your network and component wiring):
#
# * SPEAKER_IP_COMMAS: IP address of Sonos speaker you want to control,
#                      delimited by commas where you'd normally put periods
#                      (e.g. 192,168,1,149)
# * PHOTORESISTOR_PIN_NUMBER: number of the pin the photoresistor is wired to
# * BIG_LED_PIN_NUMBER: number of the pin the big LED is wired to
# * LITTLE_LED_PIN_NUMBER: number of the pin the little LED is wired to
#
#
# Optional flags:
#
# * DEBUG: verbose serial output
# * DRY_RUN: disable network and Sonos, just LED and serial output
CXXFLAGS += \
	-DSPEAKER_IP_COMMAS=192,168,1,149 \
	-DPHOTORESISTOR_PIN_NUMBER=0 \
	-DBIG_LED_PIN_NUMBER=7 \
	-DLITTLE_LED_PIN_NUMBER=3

include /usr/share/arduino/Arduino.mk
