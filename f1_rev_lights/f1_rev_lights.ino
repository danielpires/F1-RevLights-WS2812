#include <Arduino.h>
#ifdef ESP8266 
       #include <ESP8266WiFi.h>
#endif 
#ifdef ESP32   
       #include <WiFi.h>
#endif

#include <WiFiUdp.h>

#include <FastLED.h>

#define NUM_LEDS          60                   // how much LEDs are on the stripe
#define LED_PIN           12                   // LED stripe is connected to PIN D6

CRGB leds[NUM_LEDS];

#define STASSID "WIFI_NAME"
#define STAPSK  "WIFI_PASS"

#define BAUD_RATE         9600                // Change baudrate to your need

const char* ssid = STASSID;
const char* password = STAPSK;

#pragma pack(push)
#pragma pack(1)
struct PacketHeader
{
    uint16    m_packetFormat;            // 2021
    uint8     m_gameMajorVersion;        // Game major version - "X.00"
    uint8     m_gameMinorVersion;        // Game minor version - "1.XX"
    uint8     m_packetVersion;           // Version of this packet type,
                                         // all start from 1
    uint8     m_packetId;                // Identifier for the packet type,
                                         // see below
    uint64    m_sessionUID;              // Unique identifier for the session
    float     m_sessionTime;             // Session timestamp
    uint32    m_frameIdentifier;         // Identifier for the frame the data
                                         // was retrieved on
    uint8     m_playerCarIndex;          // Index of player's car in the array
    uint8     m_secondaryPlayerCarIndex; // Index of secondary player's car in
                                         // the array (split-screen)
                                         // 255 if no second player
};

struct CarTelemetryData
{
    uint16    m_speed;                    // Speed of car in kilometres per hour
    float     m_throttle;                 // Amount of throttle applied (0.0 to 1.0)
    float     m_steer;                    // Steering (-1.0 (full lock left) to 1.0 (full lock right))
    float     m_brake;                    // Amount of brake applied (0.0 to 1.0)
    uint8     m_clutch;                   // Amount of clutch applied (0 to 100)
    int8      m_gear;                     // Gear selected (1-8, N=0, R=-1)
    uint16    m_engineRPM;                // Engine RPM
    uint8     m_drs;                      // 0 = off, 1 = on
    uint8     m_revLightsPercent;         // Rev lights indicator (percentage)
    uint16    m_revLightsBitValue;        // Rev lights (bit 0 = leftmost LED, bit 14 = rightmost LED)
    uint16    m_brakesTemperature[4];     // Brakes temperature (celsius)
    uint8     m_tyresSurfaceTemperature[4]; // Tyres surface temperature (celsius)
    uint8     m_tyresInnerTemperature[4]; // Tyres inner temperature (celsius)
    uint16    m_engineTemperature;        // Engine temperature (celsius)
    float     m_tyresPressure[4];         // Tyres pressure (PSI)
    uint8     m_surfaceType[4];           // Driving surface, see appendices
};

struct PacketCarTelemetryData
{
    PacketHeader      m_header;       // Header

    CarTelemetryData    m_carTelemetryData[22];

    uint8               m_mfdPanelIndex;       // Index of MFD panel open - 255 = MFD closed
                                               // Single player, race – 0 = Car setup, 1 = Pits
                                               // 2 = Damage, 3 =  Engine, 4 = Temperatures
                                               // May vary depending on game mode
    uint8               m_mfdPanelIndexSecondaryPlayer;   // See above
    int8                m_suggestedGear;       // Suggested gear for the player (1-8)
                                               // 0 if no gear suggested
};
#pragma pack(pop)

// buffer for receiving data
uint8_t packetBuffer[9999]; //buffer to hold incoming packet,


WiFiUDP Udp;
unsigned int localPort = 20777; 

void setup() {
  Serial.begin(BAUD_RATE);
  setupWiFi();
  Udp.begin(localPort);
  
  setupFastLED();
  
}

void loop() {

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    // read the packet into packetBufffer
    Udp.read(packetBuffer, packetSize);

    struct PacketHeader *header = (struct PacketHeader *)&packetBuffer;

    uint8 myCar = header->m_playerCarIndex;
    uint8 packetId = header->m_packetId;
    
    if (packetId == 6) {    
      struct PacketCarTelemetryData *packet = (struct PacketCarTelemetryData *)&packetBuffer;
      struct CarTelemetryData *telemetry = &(packet->m_carTelemetryData[myCar]);
          
      Serial.println(telemetry->m_revLightsPercent);
      setRev(telemetry->m_revLightsPercent);
    }
    
  }
  
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.println(localIP);
}

void setupFastLED() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(0);
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
}

void setRev(float percentage) {
  
    int numLedsToFill = (NUM_LEDS * percentage) / 100;
    
    FastLED.setBrightness(255);
    for(int i = 0; i<numLedsToFill; i++){
       if(i <=20){
        leds[i] = CRGB::Green;
       }else if(i <=40){
        leds[i] = CRGB::Red;
       }else{
        leds[i] = CRGB::Purple;
       }
    }
    for(int i = numLedsToFill; i<NUM_LEDS; i++){
       leds[i] = CRGB::Black;
    }
    FastLED.show();
}
