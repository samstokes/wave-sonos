#include <Ethernet.h>
#include <SonosUPnP.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte DUMMY_MAC[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

const IPAddress SONOS_IP(192, 168, 1, 143);

const byte LED_PIN = 7;
const byte PHOTORESISTOR_PIN = 0;

void ethConnectError()
{
  Serial.println(F("Sonos client connection error (possibly harmless)"));
}

#ifndef DRY_RUN
EthernetClient ethernet;
SonosUPnP* sonos;
#endif

void printIP(IPAddress ip) {
  for (byte i = 0; i < 4; ++i) {
    Serial.print(ip[i], DEC);
    Serial.print(".");
  }
}

void setupDHCP() {
  bool ethernetted = 0;

  while (!ethernetted) {
    Serial.println(F("Attempting DHCP..."));
    ethernetted = Ethernet.begin(DUMMY_MAC) != 0;

    if (ethernetted) {
      Serial.println(F("DHCP succeeded :)"));
      Serial.print(F("My IP address: ") );
      printIP(Ethernet.localIP());
      Serial.println();
    } else {
      Serial.println(F("DHCP failed :(.  Retrying after 10 seconds..."));
      delay(10000);
    }
  }
}

void printSonosIP() {
  Serial.print(F("Sonos IP address: ") );
  printIP(SONOS_IP);
  Serial.println();
}

void setLed(byte value) {
  digitalWrite(LED_PIN, value);
#ifdef DEBUG
  Serial.print("LED: ");
  Serial.println(value, DEC);
#endif
}

void ledOn() {
  setLed(HIGH);
}

void ledOff() {
  setLed(LOW);
}

const long TIME_WINDOW = 8000L;
const long SAMPLING_INTERVAL = 100L;
const long NUM_SAMPLES = TIME_WINDOW / SAMPLING_INTERVAL;
typedef short SampleIndex;

short lightSamples[NUM_SAMPLES];
long lightTotal;
SampleIndex sampleIndex = 0;

short getLightLevel() {
  short level = analogRead(PHOTORESISTOR_PIN);
  lightSamples[sampleIndex] = level;
  SampleIndex nextSampleIndex = (sampleIndex + 1) % NUM_SAMPLES;
  lightTotal = lightTotal + level - lightSamples[nextSampleIndex];
  sampleIndex = nextSampleIndex;
  if (
#ifdef DEBUG
      true
#else
      0 == sampleIndex
#endif
     ) {
    Serial.print(F("Light level: "));
    Serial.println(level, DEC);
    Serial.print(F("Average over time window: "));
    Serial.println(lightTotal / NUM_SAMPLES, DEC);
  }
  return level;
}

void setup() {
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(9600);

#ifndef DRY_RUN
  setupDHCP();

  printSonosIP();
  sonos = new SonosUPnP(ethernet, ethConnectError);
#endif
}

byte on;

void loop() {
  byte toggle = !on;

  while (getLightLevel() * (NUM_SAMPLES * 3L) < lightTotal * 2L) {
    if (on != toggle) {
      on = toggle;
      setLed(on ? HIGH : LOW);
      Serial.println(on ? F("On") : F("Off"));

#ifndef DRY_RUN
      if (on) {
        sonos->play(SONOS_IP);
      } else {
        sonos->pause(SONOS_IP);
      }
#endif
    }
    delay(SAMPLING_INTERVAL);
  }

  delay(SAMPLING_INTERVAL);
}
