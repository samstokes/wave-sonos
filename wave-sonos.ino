#include <Ethernet.h>
#include <SonosUPnP.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte DUMMY_MAC[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

const IPAddress SONOS_IP(192, 168, 1, 143);
bool on;

const byte LITTLE_LED_PIN = 6;
const byte BIG_LED_PIN = 7;
const byte PHOTORESISTOR_PIN = 0;

const long SONOS_STATUS_POLL_DELAY = 5000L;
long sonosLastStateUpdate;

const long TIME_WINDOW = 8000L;
const long SAMPLING_INTERVAL = 100L;
const long NUM_SAMPLES = TIME_WINDOW / SAMPLING_INTERVAL;
typedef short SampleIndex;
/*
 * Ignore average light thresholds below this, to prevent oversensitivity
 * in dark conditions.  This is mainly to prevent the indicator LEDs from
 * themselves triggering the threshold detection and creating a loop.
 */
const short MIN_THRESHOLD = NUM_SAMPLES * 100;

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

void setLittleLed(byte value) {
  digitalWrite(LITTLE_LED_PIN, value);
#ifdef DEBUG
  Serial.print("Little LED: ");
  Serial.println(value, DEC);
#endif
}

void dimLittleLed() {
  analogWrite(LITTLE_LED_PIN, 0x40);
#ifdef DEBUG
  Serial.println("Little LED: dim");
#endif
}

void setBigLed(byte value) {
  digitalWrite(BIG_LED_PIN, value);
#ifdef DEBUG
  Serial.print("Big LED: ");
  Serial.println(value, DEC);
#endif
}

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

void setOn(bool newOn) {
  if (on != newOn) {
    on = newOn;
    setBigLed(on ? HIGH : LOW);
  }
}

void syncSonosState() {
  sonosLastStateUpdate = millis();
#ifdef DRY_RUN
  Serial.println(F("This is where I would sync the playing state"));
#else
  bool newOn = sonos->getState(SONOS_IP) == SONOS_STATE_PLAYING;
  Serial.print(F("Current state: "));
  Serial.println(newOn, BIN);
  setOn(newOn);
#endif
}

void setup() {
  pinMode(LITTLE_LED_PIN, OUTPUT);
  pinMode(BIG_LED_PIN, OUTPUT);

  Serial.begin(9600);

#ifndef DRY_RUN
  setupDHCP();

  printSonosIP();
  sonos = new SonosUPnP(ethernet, ethConnectError);

  syncSonosState();
#endif
}

long thresholdAtDrop = -1L;
SampleIndex indexAtSustainedDrop = -1;

bool seenDrop() { return thresholdAtDrop >= 0L; }
void setSeenDrop() {
  thresholdAtDrop = lightTotal;
  setLittleLed(HIGH);
}
void clearSeenDrop() {
  thresholdAtDrop = -1L;
  setLittleLed(LOW);
}

bool seenSustainedDrop() { return indexAtSustainedDrop >= 0; }
bool shouldForgetSustainedDrop() {
  return sampleIndex == indexAtSustainedDrop;
}
void setSeenSustainedDrop() {
  indexAtSustainedDrop = sampleIndex;
  dimLittleLed();
}
void clearSeenSustainedDrop() {
  indexAtSustainedDrop = -1;
  setLittleLed(LOW);
}

bool belowThreshold(short level, long thresholdTotal) {
  return level * (NUM_SAMPLES * 3L) < thresholdTotal * 2L;
}

void loop() {
  long now = millis();
  if (sonosLastStateUpdate > now || now > sonosLastStateUpdate + SONOS_STATUS_POLL_DELAY) {
    syncSonosState();
  }

  short level = getLightLevel();

  if (seenDrop()) {
    if (!belowThreshold(level, thresholdAtDrop)) {
      Serial.print(F("Light level "));
      Serial.print(level, DEC);
      Serial.print(F(" rose back above average of "));
      Serial.println(thresholdAtDrop / NUM_SAMPLES, DEC);

      setOn(!on);
      Serial.println(on ? F("On") : F("Off"));

      clearSeenDrop();

#ifndef DRY_RUN
      sonos->togglePause(SONOS_IP);
#endif
    } else if (!belowThreshold(level, lightTotal)) {
      Serial.print(F("Average light level "));
      Serial.print(lightTotal / NUM_SAMPLES, DEC);
      Serial.print(F(" dropped to near light level "));
      Serial.print(level, DEC);
      Serial.println(F(" so assuming the room lights went out"));

      clearSeenDrop();
      setSeenSustainedDrop();
    }
  } else if (seenSustainedDrop()) {
    if (shouldForgetSustainedDrop()) {
      Serial.println(F("Time to forget the room lights going out"));

      clearSeenSustainedDrop();
    }
  } else {
    if (belowThreshold(level, lightTotal)) {
      Serial.print(F("Light level "));
      Serial.print(level, DEC);
      Serial.print(F(" dropped below average of "));
      Serial.println(lightTotal / NUM_SAMPLES, DEC);

      if (lightTotal >= MIN_THRESHOLD) {
        setSeenDrop();
      } else {
        Serial.print(F("Ignoring as average is below "));
        Serial.println(MIN_THRESHOLD / NUM_SAMPLES, DEC);
      }
    }
  }

  delay(SAMPLING_INTERVAL);
}
