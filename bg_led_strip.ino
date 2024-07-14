/*
D1 Mini ESP8266 and WS2182b project that fetches bg data from Nightscout
and displays the bg level as colours on the led-strip.
Yellow: High
Green: Good
Red: Low
Based on mmol/L. Conversion is done in the main loop()

BOARD: Generic ESP8266 Module
*/

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

// Neopixel uses the GPIO number, not BCM.
#define D1 5
#define LED_PIN D1

// How many NeoPixels are attached to the ESP8266?
#define LED_COUNT 10

// NeoPixel brightness, 0 (min) to 255 (max)
// Set BRIGHTNESS to about 1/5 (max = 255)
#define BRIGHTNESS 150

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

String ssid = SECRET_SSID;
String password = SECRET_PSWD;

String server  = SECRET_HOMEPAGE + String("/api/v1/");
String entries = "entries?count=1";
String token = SECRET_TOKEN;

String getFromApi = server +  entries + "&" + "token=" + token;

const int error  = strip.Color(126, 0, 126);
const int white  = strip.Color(255, 255, 255);
const int red    = strip.Color(255, 0, 0);
const int yellow = strip.Color(255, 255, 0);
const int green  = strip.Color(0, 255, 0);

void fill_array(uint32_t colour) {
  // This function takes 50 * numPixels to complete
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, colour);
    strip.show();
    delay(50);
  }
}

void update_strip_pattern(uint32_t colour) {
  // This function updates the leds in a pattern. If you want something simpler, use fill_array directly
  // in display_bg_level function instead.
  fill_array(white);
  // Note: Add 50 * numPixels to total delay
  // Delay for one second before updating to give people time
  // to notice that an update is coming.
  delay(500);
  // Note: Add 500 to total delay
  for (int i = 0; i < strip.numPixels(); i++) {
    for (int j = 0; j < strip.numPixels() - i; j++) {
      // Loop over the led array. First set the first LED to colour,
      // then set the second LED to colour (and set the first to white again)
      strip.setPixelColor(j, colour);
      if (j > 0) {
        strip.setPixelColor(j - 1, white);
      }
      strip.show();
      delay(50);
      // Note: Add 50 * numPixels - i to total delay
      total_delay += 50;
    }
    delay(50 * i);
    // Note: Add 50 * i to total delay
  }
  // Note: Total delay: 500 + 2 * 50 * numPixels + 50 * (i - i) = 500 + 100*numPixels
  // This might be off, but this was the intention anyway.
}

void display_bg_level(float bg) {
  if (bg >= 10.0) {
    Serial.println("Setting YELLOW");
    update_strip_pattern(yellow);
  } else if (bg >= 4.0) {
    Serial.println("Setting GREEN");
    update_strip_pattern(green);
  } else {
    Serial.println("Setting RED");
    update_strip_pattern(red);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("STARTING");

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  fill_array(error);
  strip.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();   // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);

  // Initialize Wi-Fi
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println('.');
    delay(500);
  }
  Serial.println("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  fill_array(white);
}

void loop() {
  // This is the main loop which runs every 5 (ish) minutes.
  // The interval is set because that's how often dexcom g6 updates values on the nightscout page.
  WiFiClientSecure client;

  if ((WiFi.status() == WL_CONNECTED)) {
    // Set insecure client with certificate because I couldn't get it to work with the cert
    client.setInsecure();
    // Create an HTTPClient instance
    HTTPClient https;

    // Initializing HTTPS communication using the insecure client
    Serial.print("[HTTPS] begin...\n");
    Serial.println(getFromApi);

    if (https.begin(client, getFromApi.c_str())) {
      https.addHeader("accept", "application/json");

      Serial.print("[HTTPS] GET...\n");
      // Start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been sent and the server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // Response received from server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          // Get payload.
          String payload = https.getString();
          Serial.println("PAYLOAD:");
          Serial.println(payload);
          // The size here is based on the standard size of data received. It might need
          // to be increased.
          StaticJsonDocument<768> doc;
          auto error = deserializeJson(doc, payload);
          if (error) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
            return;
          } else {
            Serial.println("JSON OBJECT:");
            JsonObject obj = doc[0];
            // The part of the json data that is interresting is the sgv field. Convert it to mmol/L.
            float bg = obj["sgv"].as<float>() / 18;
            display_bg_level(bg);
          }
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    }
    // 5 minute delay until next fetch.
    // TODO: This timer might need to be updated since a few seconds pass in the updateStrip function. This
    //       causes a time drift meaning an occasional missed BG update might occur.
    // display_bg_level takes 500 + 100*numPixels to complete, so subtract that here.
    delay(1000 * 60 * 5 - 500 - 100 * LED_COUNT);
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
    // Set the colour of the strip to something not used otherwise to indicate that an error occured.
    fill_array(error);
    delay(1000 * 60 * 1); // 1 minute delay on fail to connect.
  }
}
