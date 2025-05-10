#include <FastLED.h>
// =============================================================================================================
// =================================================== SETTINGS ================================================
// =============================================================================================================


// Режим подключения Wi-Fi
const char *ssid = "NAMEEEEEEEEEEEEEEEEEEEE";    // Имя домашней Wi-Fi сети
const char *password = "PASSSSSSSSSSSSSSS"; // Пароль для доступа к домашней сети


// Определение домашнего региона (областей), они будут мигать и оповещять если это включено
const int homeRegions[] = {24, 2}; // поспотреть нумерацию областей можно на +- 46 строке

bool isAlarm = false; // звуковое оповещение по домашнему региону


// Определение констант 
#define BTN_PIN 4    // Пин, к которому подключена кнопка GND - BTN - PIN
#define BUZ_PIN 12  // Пин, к которому подключена пищялка
#define DATA_PIN 2 // Пин, к которому подключена светодиодная лента


// =============================================================================================================
// ================================================ FOR DEVELOPERS =============================================
// =============================================================================================================

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>



#define NUM_LEDS 27
#define UPDATES_PER_SECOND 100


CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myWhiteBluePalette;
extern const TProgmemPalette16 myWhiteBluePalette_p PROGMEM;

const char *regionNames[] = {
  //  0                   1                    2
  "Луганська область", "Сумська область", "Харківська область",
  //  3                   4                        5
  "Донецька область", "Запорізька область", "Дніпропетровська область",
  //  6                       7                       8
  "Полтавська область", "Чернігівська область", "Черкаська область",
  //  9                          10                    11
  "Кіровоградська область", "Миколаївська область", "Херсонська область",
  //  12                          13                  14
  "Автономна Республіка Крим", "Одеська область", "Вінницька область",
  //  15            16                  17                      18
  "м. Київ", "Київська область", "Житомирська область", "Хмельницька область",
  // 19                       20                          21
  "Чернівецька область", "Івано-Франківська область", "Тернопільська область",
  //        22                  23                  24
  "Рівненська область", "Волинська область", "Львівська область",
   //    25
  "Закарпатська область"
};

const char *serverUrl = "http://ubilling.net.ua/aerialalerts";
int regionIndices[NUM_LEDS], numActiveRegions = 0, buttonState, lastButtonState = LOW, deMode = 0;
unsigned long lastDebounceTime = 0, debounceDelay = 50, startTime, holdStartTime = 0, previousMillis = 0, previousMillisBlink = 0, previousMillisRate = 0, previousMillisAlarm = 0, previousMillisAnim = 0;  // переменная для хранения предыдущего времени
const int interval = 6000, intervalRate = 500, intervalBlink = 700, intervalAlarm = 5000;        // интервал времени в миллисекундах (1000 миллисекунд = 1 секунда)
bool isButtonHeld = false, dangerHomeReg = false;


void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    blinkEffect(CRGB::Blue, CRGB::Yellow, 100);
  }
  currentPalette = myWhiteBluePalette_p;
  currentBlending = LINEARBLEND;
}

void loop() {
  btnTick();

  switch (deMode) {
    case 0:
      mapOneAlarm();
      break;
    case 1:
      colorModeL();
      break;
  }
  
}

void blinkEffect(CRGB color1, CRGB color2, int delayTime) {
  fill_solid(leds, NUM_LEDS / 2, color1);
  fill_solid(leds + NUM_LEDS / 2, NUM_LEDS / 2, color2);
  FastLED.show();
  delay(delayTime);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(delayTime);
}

void colorModeL() {
  ChangePalettePeriodically();

  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors( startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for ( int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void ChangePalettePeriodically()
{
  uint8_t secondHand = (millis() / 1000) % 25;
  static uint8_t lastSecond = 30;

  if ( lastSecond != secondHand) {
    lastSecond = secondHand;
    if ( secondHand == 0)  {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = NOBLEND;
    }
    if ( secondHand == 10)  {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 20)  {
      currentPalette = myWhiteBluePalette_p;
      currentBlending = NOBLEND;
    }
    if ( secondHand == 35)  {
      currentPalette = myWhiteBluePalette_p;
      currentBlending = LINEARBLEND;
    }
  }
}

void SetupTotallyRandomPalette()
{
  for ( int i = 0; i < 16; ++i) {
    currentPalette[i] = CHSV( random8(), 255, random8());

    currentPalette[0] = CRGB::White;
    currentPalette[1] = CRGB::White;

  }
}

void SetupBlackAndWhiteStripedPalette()
{

  fill_solid( currentPalette, 16, CRGB::Black);

  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;

}

const TProgmemPalette16 myWhiteBluePalette_p PROGMEM =
{

  CRGB::Yellow, 
  CRGB::Blue,
  CRGB::Black,


  CRGB::Yellow,
  CRGB::Blue,
  CRGB::Black,


  CRGB::Yellow,
  CRGB::Yellow,
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black
};

bool contains(const int arr[], int size, int value) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == value) {
      return true;
    }
  }
  return false;
}

void mapOneAlarm() {
  if (millis() - previousMillisRate >= intervalRate) {
    drawLEDs();
    previousMillisRate = millis();
  }

  if (millis() - previousMillis >= interval) {
    getJsonData();

    previousMillis = millis();
  }
  if (dangerHomeReg && isAlarm && millis() - previousMillisAlarm >= intervalAlarm) {
    Alarm();

    previousMillisAlarm = millis();
  }
}

void btnTick() {
  int reading = digitalRead(BTN_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == HIGH) {
        // Кнопка была нажата
        holdStartTime = millis();
        isButtonHeld = false;
      } else {
        // Кнопка была отпущена
        if (!isButtonHeld) {
          // Если кнопка не была удержана, считаем это кликом
          Serial.println("b0 click");
          btnClick();
        }
      }
    }

    // Проверяем удержание
    if (buttonState == HIGH && (millis() - holdStartTime) > 3000) {
      // Удержание длилось более 1 секунды
      isButtonHeld = true;
      Serial.println("b0 hold");
      btnHold();
    }
  }

  lastButtonState = reading;
}

void btnHold() {

  isAlarm = !isAlarm;
  ConfirmationSound(isAlarm);
  delay(500);


}

void ConfirmationSound(bool isOn) {
  startTime = millis();  // Запоминаем время начала функции.
  if (isOn) {


    while (millis() - startTime < 200) {
      tone(BUZ_PIN, 1000, 200);  // Выводим звуковой сигнал с частотой 2048 Гц и длительностью 0,5 сек.
    }

    startTime = millis();  // Сбрасываем время для следующего звука.

    while (millis() - startTime < 200) {
      // Пауза между звуками.
    }

    startTime = millis();  // Сбрасываем время для следующего звука.

    while (millis() - startTime < 200) {
      tone(BUZ_PIN, 1200, 200);  // Выводим звуковой сигнал длительностью 0,5 сек с частотой 1024 Гц.
    }



  } else {
    // Звук подтверждения выключения оповещения.
    while (millis() - startTime < 200) {
      tone(BUZ_PIN, 1200, 200); // Выводим звуковой сигнал с частотой 2048 Гц и длительностью 0,5 сек.
    }

    startTime = millis();  // Сбрасываем время для следующего звука.

    while (millis() - startTime < 200) {
      // Пауза между звуками.
    }

    startTime = millis();  // Сбрасываем время для следующего звука.

    while (millis() - startTime < 200) {
      tone(BUZ_PIN, 1000, 200);  // Выводим звуковой сигнал длительностью 0,5 сек с частотой 1024 Гц.
    }


  }

}

void btnClick() {
  deMode = (deMode + 1) % 2;

}

void drawLEDs() {

  fill_solid(leds, NUM_LEDS, CRGB::ForestGreen); //LightSeaGreen

  for (int i = 0; i < numActiveRegions; i++) {
    leds[regionIndices[i]] = CRGB::Red;
  }



  if (millis() - previousMillisBlink >= intervalBlink) {
    dangerHomeReg = false;
    for (int i = 0; i < sizeof(homeRegions) / sizeof(homeRegions[0]); i++) {
      for (int j = 0; j < sizeof(regionIndices) / sizeof(regionIndices[0]); j++) {
        if (homeRegions[i] == regionIndices[j]) {
          leds[homeRegions[i]] = CRGB::Black;
          dangerHomeReg = true;


        }
      }

    }

    previousMillisBlink = millis();
  }
  FastLED.show();
}

void getJsonData() {
  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverUrl);

  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    String payload = http.getString();
    processJsonData(payload);

  }

  http.end();
}

void processJsonData(String jsonData) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonData);

  // Reset active regions
  
  for (int i = 0; i <= numActiveRegions; i++){
    regionIndices[i]=0;
  }
  
  numActiveRegions = 0;
  
for (JsonPair region : doc["states"].as<JsonObject>()) {
  JsonObject regionData = region.value().as<JsonObject>();
  bool regionState = regionData["alertnow"];
  if (regionState) {
    String regionName = String(region.key().c_str());
    int regionIndex = mapRegionToIndex(regionName);
    if (regionIndex != -1 && numActiveRegions < NUM_LEDS) {
      regionIndices[numActiveRegions++] = regionIndex;
    }
  }
}
}

void Alarm() {
  startTime = millis();  // Запоминаем время начала функции.

  while (millis() - startTime < 200) {
    tone(BUZ_PIN, 2048, 200);  // Выводим звуковой сигнал с частотой 2048 Гц и длительностью 0,5 сек.
  }

  startTime = millis();  // Сбрасываем время для следующего звука.

  while (millis() - startTime < 200) {
    // Пауза между звуками.
  }

  startTime = millis();  // Сбрасываем время для следующего звука.

  while (millis() - startTime < 200) {
    tone(BUZ_PIN, 1024, 200);  // Выводим звуковой сигнал длительностью 0,5 сек с частотой 1024 Гц.
  }

  startTime = millis();  // Сбрасываем время для следующего звука.

  while (millis() - startTime < 200) {
    // Пауза между звуками.
  }

  startTime = millis();  // Сбрасываем время для следующего звука.

  while (millis() - startTime < 200) {
    tone(BUZ_PIN, 512, 200);  // Выводим звуковой сигнал длительностью 0,5 сек с частотой 512 Гц.
  }

  startTime = millis();  // Сбрасываем время для следующего звука.

  while (millis() - startTime < 1000) {
    // Пауза между звуками.
  }
}

int mapRegionToIndex(String regionName) {
  for (int i = 0; i < sizeof(regionNames) / sizeof(regionNames[0]); i++) {
    if (regionName == regionNames[i]) return i;
  }
  return -1;
}




