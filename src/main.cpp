#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SimpleKalmanFilter.h>
#include "Adafruit_seesaw.h"
#include <ShiftRegister74HC595.h>
#include <jled.h>
#include <PinButton.h>
#include <TelnetPrint.h>

#include "credentials.h"
#include "helpers.h"
#include "global.h"
#include "database.h"
#include "mqtt.h"

#include "Page_Admin.h"
#include "Page_Script.js.h"
#include "Page_Style.css.h"
#include "Page_NTPsettings.h"
#include "Page_Information.h"
#include "PAGE_NetworkConfiguration.h"

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting ESP32");
  JLed(LED_PIN).On().Update();
  EEPROM.begin(1024);

  WiFi.disconnect(true); // delete old config
  delay(1000);
  WiFi.onEvent(WiFiEvent); //register event handler

  if (!readConfig())
  {
    // DEFAULT CONFIG

    networkConfig.ssid = "SSID";
    networkConfig.password = "Password";
    networkConfig.dhcp = true;
    networkConfig.IP[0] = 192;
    networkConfig.IP[1] = 168;
    networkConfig.IP[2] = 0;
    networkConfig.IP[3] = 100;
    networkConfig.Netmask[0] = 255;
    networkConfig.Netmask[1] = 255;
    networkConfig.Netmask[2] = 255;
    networkConfig.Netmask[3] = 0;
    networkConfig.Gateway[0] = 192;
    networkConfig.Gateway[1] = 168;
    networkConfig.Gateway[2] = 0;
    networkConfig.Gateway[3] = 1;
    networkConfig.ntpServerName = "1.ar.pool.ntp.org";
    networkConfig.Update_Time_Via_NTP_Every = 5;
    networkConfig.timezone = -30;
    networkConfig.daylight = true;
    networkConfig.fbEmail = "Email";
    networkConfig.fbPassword = "Password";

    systemConfig.airValue = AIR_VALUE;
    systemConfig.waterValue = WATER_VALUE;
    systemConfig.dryLimit = 50;
    systemConfig.wetLimit = 90;
    systemConfig.sensorMode = true;
    systemConfig.autoMode = true;

    scheduleConfig.water1StartHour = 6;
    scheduleConfig.water1StartMinute = 0;
    scheduleConfig.water2StartHour = 12;
    scheduleConfig.water2StartMinute = 0;
    scheduleConfig.wdayOn = B1111111;

    state.waterState = OFF;
    state.soilState = DRY;
    state.currentTurn = 0;
    state.currentZone = 0;
    state.soilMoistInit = 0;
    state.currentWaterStartTimestamp = 0;
    state.currentWaterEndTimestamp = 0;

    error.sensorOutOfRange = false;
    error.sensorNotAnswering = false;

    for (int i = 0; i < MAX_ZONE_NUMBER; i++)
    {
      zoneConfig.zone[i].waterAuto = true;
      zoneConfig.zone[i].waterOrder = i;
      zoneConfig.zone[i].waterCapacity = 10;
      zoneConfig.zone[i].waterQ = 5;
      zoneConfig.zone[i].waterQMax = 10;
      // zoneConfig.zone[i].name = "zone" + (String)(i + 1);
    }

    writeConfig();
    Serial.println("General config applied");
  }

  tkSecond.attach(1, Second_Tick);

  // pinMode(BUTTON_PIN, INPUT_PULLUP);
  if (!digitalRead(BUTTON_PIN)) //OTA Mode?
  {
    Serial.println("OTA READY");
    otaFlag = true;
  }
  else
    clientInit();

  pinMode(RELAY_OUTPUT_ENABLE, OUTPUT);
  digitalWrite(RELAY_OUTPUT_ENABLE, LOW);

  uint8_t outputValues[] = {B00000000};
  setRelays(outputValues);

  /* Set all relay outputs LOW
* 0: Tank Valve
* 1: Zone 1 Valve
* ...
* MAX_ZONE_NUMBER: Zone MAX_ZONE_NUMBER Valve
* ...
* MSB: Pump
*/
  pinMode(TANK_LEVEL_PIN, INPUT_PULLUP);

  state.waterState = OFF;
}

void loop()
{
  yield();
  button.update();
  ledSequence.Update();

  if (boardMode == 'C')
  {
    server.handleClient();
    if (adminTimeOutCounter > ADMIN_TIMEOUT || button.isLongClick())
    {
      boardMode = 'N';
      Serial.println("Normal operation mode set.");
      
      JLed(LED_PIN).Off().Update();
      clientInit();
    }
  }
  else
  {
    checkTankLevel();
    getMeasures();
    if (WiFiConnected)
    {
      handleMqtt();
      getNTP();
      if (databaseConnected)
        setMeasures();
      else
        reconnectDatabase();
    }
    event = getNewEvent();
    state_table[season][state.soilState][state.waterState][event]();
    // if (event != 0 && event != 1 && event != 2)
    // {
    //   Serial.println("season: " + (String)season);
    //   Serial.println("soilState: " + (String)state.soilState);
    //   Serial.println("waterState: " + (String)state.waterState);
    //   Serial.println("event: " + (String)event);
    // }
  }
  customWatchdog = millis();
}

// Input

void checkAlarms()
{

  if (systemConfig.autoMode)
  {
    // Serial.println(("Auto Water Mode is ON"));
    if (scheduleConfig.wdayOn & 1 << (DateTime.wday - 1))
    {
      // Serial.println((String)DateTime.wday+" day is ON");
      // Serial.println((String)DateTime.hour+":"+(String)DateTime.minute);
      if (DateTime.hour == scheduleConfig.water1StartHour && DateTime.minute == scheduleConfig.water1StartMinute)
        alarm1State = true;
      if (DateTime.hour == scheduleConfig.water2StartHour && DateTime.minute == scheduleConfig.water2StartMinute)
        alarm2State = true;
    }
  }
}

void checkTimer()
{
  if (state.waterState == AUTO_WATER || state.waterState == MANUAL_WATER)
  {
    if (UnixTimestamp >= state.endTimestamp[state.currentZone])
      waterNextZone();
  }
  else if (state.waterState == SINGLE_WATER)
  {
    if (UnixTimestamp >= state.currentWaterEndTimestamp)
      stopWater();
  }
}

defEvent getNewEvent()
{
  defEvent e /* = NO_EVENT */;
  if (tankRequest)
  {
    e = TANK_REQUEST;
    tankRequest = false;

    Serial.println("Tank Interrupt Request @ ");
    digitalClockDisplay();
  }
  else if (button.isSingleClick())
  {
    e = BUTTON_SINGLE;

    Serial.println("Single Button pressed @ ");
    digitalClockDisplay();
  }
  else if (button.isLongClick())
  {
    e = BUTTON_LONG;

    Serial.println("Long Button pressed @ ");
    digitalClockDisplay();
  }
  else if (appEvent)
  {
    e = APP_EVENT;
    appEvent = false;

    Serial.println("Manual Test @ ");
    digitalClockDisplay();
  }
  else if (alarm1State)
  {
    e = ALARM_1;
    alarm1State = false;
  }
  else if (alarm2State)
  {
    e = ALARM_2;
    alarm2State = false;
  }
  else
  {
    e = NO_EVENT;
  }
  return e;
}

void getMeasures() // Read sensors timer
{
  unsigned long currentMillisGetMeasures = millis();
  if (currentMillisGetMeasures - previousMillisGetMeasures > intervalGetMeasures)
  {
    previousMillisGetMeasures = currentMillisGetMeasures; // save the last publish time
    if (soilSensor.begin(0x36))
    {
      if (error.sensorNotAnswering)
      {
        error.sensorNotAnswering = LOW;
        if (databaseConnected)
          setWarningAlarms();
      }
      readSensor();
    }
    else
    {
      if (!error.sensorNotAnswering)
      {
        error.sensorNotAnswering = HIGH;
        if (databaseConnected)
          setWarningAlarms();
      }
    }
  }
}

void readSensor()
{
  if (!error.sensorNotAnswering)
  {
    // Serial.println("Reading sensor...");
    measuredSoilSensorValue = soilSensor.touchRead(0);
    estimatedSoilSensorValue = soilMoistureKF.updateEstimate(measuredSoilSensorValue);
    measuredTemperature = soilSensor.getTemp();
    estimatedTemperature = temperatureKF.updateEstimate(measuredTemperature);

    if (systemConfig.airValue < estimatedSoilSensorValue || estimatedSoilSensorValue < systemConfig.waterValue)
    {
      soilMoisture = (100.0 * (estimatedSoilSensorValue - systemConfig.airValue)) / (systemConfig.waterValue - systemConfig.airValue);
      if (soilMoisture >= systemConfig.wetLimit)
      { // Soil is Very Wet
        state.soilState = VERY_WET;
      }
      else if (soilMoisture >= systemConfig.dryLimit)
      { // Soil is Wet
        state.soilState = WET;
      }
      else
      { // Soil is Dry
        state.soilState = DRY;
      }

      if (error.sensorOutOfRange)
      {
        error.sensorOutOfRange = LOW;
        if (databaseConnected)
          setWarningAlarms();
      }
    }
    else
    {
      error.sensorOutOfRange = HIGH;
      if (databaseConnected)
        setWarningAlarms();
    }
  }
}

float filterCapSensorData()
{
  float _measuredCapSensorData = 0, _estimatedCapSensorData = 0;
  for (int i = 0; i < 200; i++)
  {
    _measuredCapSensorData = soilSensor.touchRead(0);
    _estimatedCapSensorData = soilMoistureKF.updateEstimate(_measuredCapSensorData);
    delay(20);
  }
  soilMoisture = (100.0 * (_estimatedCapSensorData - systemConfig.airValue)) / (systemConfig.waterValue - systemConfig.airValue);
  Serial.print("Estimated Soil Sensor Data: ");
  Serial.println(_estimatedCapSensorData);
  return _estimatedCapSensorData;
}

float filterTempSensorData()
{
  float _measuredTempSensorData = 0, _estimatedTempSensorData = 0;
  for (int i = 0; i < 200; i++)
  {
    _measuredTempSensorData = soilSensor.getTemp();
    _estimatedTempSensorData = temperatureKF.updateEstimate(_measuredTempSensorData);
    delay(20);
  }
  estimatedTemperature = _estimatedTempSensorData;
  Serial.print("Estimated Temperaure Sensor Data: ");
  Serial.println(_estimatedTempSensorData);
  return _estimatedTempSensorData;
}

// Output
void doNothing()
{
}

void waterAuto()
{
  state.waterState = AUTO_WATER;
  state.soilMoistInit = soilMoisture;
  state.currentTurn = 0;

  uint8_t outputValues[] = {B00000000};
  byte lastTurnZone;
  // byte waterDuration[MAX_ZONE_NUMBER][2];
  unsigned int waterDurationS[MAX_ZONE_NUMBER];

  Serial.print("Soil State is ");
  Serial.println((state.soilState == 0   ? "DRY"
                  : state.soilState == 1 ? "WET"
                  : state.soilState == 2 ? "VERY WET"
                                         : "UNKNOWN"));

  for (int i = 0; i < MAX_ZONE_NUMBER; i++) // Turn
  {
    for (int j = 0; j < MAX_ZONE_NUMBER; j++) // Zone
    {
      if (zoneConfig.zone[j].waterOrder == i)
      {
        if (zoneConfig.zone[j].waterAuto)
        {
          switch (state.soilState)
          {
          case DRY: //  t[hr] =  (Qmax - ( ( Qmax - Qs ) / dL ) * sM) / Cap
            waterDurationS[j] = (zoneConfig.zone[j].waterQMax - ((float)(zoneConfig.zone[j].waterQMax - zoneConfig.zone[j].waterQ) / (float)systemConfig.wetLimit) * soilMoisture) / (float)(zoneConfig.zone[j].waterCapacity) * 3600;
            break;
          case WET: //  t = (( Qs / ( wL - dL ) ) * ( wL - sM )) / Cap
            waterDurationS[j] = (((float)zoneConfig.zone[j].waterQ / (float)(systemConfig.wetLimit - systemConfig.dryLimit)) * ((float)systemConfig.wetLimit - soilMoisture)) / (float)(zoneConfig.zone[j].waterCapacity) * 3600;
            break;
          case VERY_WET: // t = 0
            waterDurationS[j] = 0;
            break;
          default:
            break;
          }

          Serial.print("Water Duration Time for Zone ");
          Serial.print(j + 1);
          Serial.print(" set to: ");
          Serial.print(waterDurationS[j]);
          Serial.println("");
          if (state.currentTurn == i && waterDurationS[j] > 0)
          {
            state.currentZone = j;
            outputValues[0] = 1 << (j + 1) | 1 << 7; // Turn current turn zone ON

            led[0] = JLed(LED_PIN).Blink(100, 200).Repeat(j + 1);
            led[1] = JLed(LED_PIN).Off().DelayAfter(1000);
          }

          if (i == 0) // 1st Turn
          {
            state.currentWaterStartTimestamp = UnixTimestamp;
            state.endTimestamp[j] = state.currentWaterStartTimestamp + waterDurationS[j];

            Serial.println("Water Start Timestamp: " + (String)state.currentWaterStartTimestamp);
          }
          else // Not 1st Turn
          {
            state.endTimestamp[j] = state.endTimestamp[lastTurnZone] + waterDurationS[j];
          }
        }
        else
        {
          state.currentTurn++;
          if (i == 0) // 1st Turn
          {
            state.currentWaterStartTimestamp = UnixTimestamp;
            Serial.println("Water Start Timestamp: " + (String)state.currentWaterStartTimestamp);

            state.endTimestamp[j] = UnixTimestamp;
          }
          else // Not 1st Turn
          {
            state.endTimestamp[j] = state.endTimestamp[lastTurnZone];
          }
        }
        Serial.print("Water End Time for Zone ");
        Serial.print(j + 1);
        Serial.print(" set to: ");
        Serial.print(state.endTimestamp[j]);
        Serial.println("");

        if (i == MAX_ZONE_NUMBER - 1) // Last Turn
        {
          state.currentWaterEndTimestamp = state.endTimestamp[j];
          Serial.println("Water End Timestamp: " + (String)state.currentWaterEndTimestamp);
        }
        lastTurnZone = j;
      }
    }
  }
  setRelays(outputValues);
  writeConfig();
  if (databaseConnected)
    setWaterStateLog();
  TelnetPrint.print("Water Auto at ");
  TelnetPrint.println(UnixTimestamp);
}

void waterSingleZone()
{
  uint8_t outputValues[] = {B00000000};
  for (int i = 0; i < MAX_ZONE_NUMBER; i++)
  {
    if (newValveState & (1 << i)) // Valve i set ON
    {
      if (!(valveState & (1 << (i + 1)))) // Valve i is OFF
      {
        unsigned long waterDuration;
        waterDuration = ((float)zoneConfig.zone[i].waterQ / (float)zoneConfig.zone[i].waterCapacity) * 3600;

        state.currentWaterStartTimestamp = UnixTimestamp;
        state.currentWaterEndTimestamp = UnixTimestamp + waterDuration;
        if (waterDuration > 0)
        {
          outputValues[0] = 1 << (i + 1) | 1 << 7;
          led[0] = JLed(LED_PIN).Blink(100, 200).Repeat(i + 1);
          led[1] = JLed(LED_PIN).Off().DelayAfter(1000);
          state.currentZone = i;
        }

        Serial.print("Water Zone ");
        Serial.print(i + 1);
        Serial.print(" Manual set ON. Water Duration Time set to: ");
        Serial.print(waterDuration);
        Serial.print(" at ");
        digitalClockDisplay();
      }
    }
  }
  setRelays(outputValues);

  if (valveState != 0)
    state.waterState = SINGLE_WATER;
  else
  {
    state.waterState = OFF;
    led[0] = JLed(LED_PIN).On().DelayAfter(1000);
    led[1] = JLed(LED_PIN).On().DelayAfter(1000);
  }
  // writeConfig();
  if (databaseConnected)
    setWaterStateLog();
  TelnetPrint.print("Water Single Zone at ");
  TelnetPrint.println(UnixTimestamp);
}

void waterManual()
{
  Serial.println("Water Manual");

  state.waterState = MANUAL_WATER;
  state.soilMoistInit = soilMoisture;
  state.currentTurn = 0;

  uint8_t outputValues[] = {B00000000};
  byte lastTurnZone;
  // byte actualTime[] = {DateTime.hour, DateTime.minute};
  unsigned long waterDuration[MAX_ZONE_NUMBER];

  for (int i = 0; i < MAX_ZONE_NUMBER; i++) // Turn
  {
    for (int j = 0; j < MAX_ZONE_NUMBER; j++) // Zone
    {
      if (zoneConfig.zone[j].waterOrder == i)
      {
        // unsigned long _waterEndTime;

        waterDuration[j] = ((float)zoneConfig.zone[j].waterQ / (float)zoneConfig.zone[j].waterCapacity) * 3600;

        // waterDuration[j][0] = waterDuration[j][1] / 60;
        // waterDuration[j][1] -= waterDuration[j][0] * 60;

        Serial.print("Water Duration Time for Zone ");
        Serial.print(j + 1);
        Serial.print(" set to: ");
        Serial.print(waterDuration[j]);
        // printDigits(waterDuration[j][1]);
        Serial.println("");

        if (state.currentTurn == i && (waterDuration[j] > 0))
        {
          state.currentZone = j;
          outputValues[0] = 1 << (j + 1) | 1 << 7; // Turn current turn zone ON

          led[0] = JLed(LED_PIN).Blink(100, 200).Repeat(j + 1);
          led[1] = JLed(LED_PIN).Off().DelayAfter(1000);
        }

        if (i == 0) // 1st Turn
        {
          state.currentWaterStartTimestamp = UnixTimestamp;
          state.endTimestamp[j] = state.currentWaterStartTimestamp + waterDuration[j];

          Serial.println("Water Start Timestamp: " + (String)state.currentWaterStartTimestamp);
          // addTime(actualTime, waterDuration[j], _waterEndTime);
        }
        else // Not 1st Turn
        {
          state.endTimestamp[j] = state.endTimestamp[lastTurnZone] + waterDuration[j];

          // byte _endTime[] = {zoneConfig.zone[lastTurnZone].waterEndHour, zoneConfig.zone[lastTurnZone].waterEndMinute};
          // addTime(_endTime, waterDuration[j], _waterEndTime);
        }
        // zoneConfig.zone[j].waterEndHour = _waterEndTime[0];
        // zoneConfig.zone[j].waterEndMinute = _waterEndTime[1];

        Serial.print("Water End Time for Zone ");
        Serial.print(j + 1);
        Serial.print(" set to: ");
        Serial.print(state.endTimestamp[j]);
        // printDigits(zoneConfig.zone[j].waterEndMinute);
        Serial.println("");

        if (i == MAX_ZONE_NUMBER - 1) // Last Turn
        {
          // byte _startTime[] = {DateTime.hour, DateTime.minute};
          // byte _endTime[] = {zoneConfig.zone[j].waterEndHour, zoneConfig.zone[j].waterEndMinute};
          // byte _totalDuration[2];
          // subtractTime(_endTime, _startTime, _totalDuration);
          state.currentWaterEndTimestamp = state.endTimestamp[j];
          Serial.println("Water End Timestamp: " + (String)state.currentWaterEndTimestamp);
        }
        lastTurnZone = j;
      }
    }
  }
  setRelays(outputValues);
  writeConfig();
  if (databaseConnected)
    setWaterStateLog();
  TelnetPrint.print("Water Manual at ");
  TelnetPrint.println(UnixTimestamp);
}

void waterNextZone()
{
  uint8_t outputValues[] = {B00000000};

  if (state.currentTurn == MAX_ZONE_NUMBER - 1) // Last Turn
  {
    stopWater();
    // state.waterState = OFF;
    // state.currentTurn = 0;
    // for (int i = 0; i < MAX_ZONE_NUMBER; i++)
    // {
    //   if (zoneConfig.zone[i].waterOrder == state.currentTurn)
    //     state.currentZone = i;
    // }
  }
  else
  {
    unsigned long remainingTime;

    for (int i = state.currentTurn; i < MAX_ZONE_NUMBER; i++)
    {
      for (int j = 0; j < MAX_ZONE_NUMBER; j++) // Zone
      {
        if (zoneConfig.zone[j].waterOrder == i)
        {
          if (state.currentTurn == i)
          {
            remainingTime = state.endTimestamp[j] - UnixTimestamp;
            state.endTimestamp[j] = UnixTimestamp;
          }
          else
          {
            state.endTimestamp[j] = state.endTimestamp[j] - remainingTime;
          }
          if (state.currentTurn + 1 == i)
          {
            outputValues[0] = 1 << (j + 1) | 1 << 7;
            state.currentZone = j;

            setRelays(outputValues);
            // writeConfig();
            if (databaseConnected)
              setWaterStateLog();
            TelnetPrint.print("Water Next Zone at ");
            TelnetPrint.println(UnixTimestamp);

            led[0] = JLed(LED_PIN).Blink(100, 200).Repeat(j + 1);
            led[1] = JLed(LED_PIN).Off().DelayAfter(1000);
          }
          Serial.print("Water End Time for Zone ");
          Serial.print(j + 1);
          Serial.print(" set to: ");
          Serial.print(state.endTimestamp[j]);
          Serial.println("");
        }
      }
    }
    state.currentTurn++;
  }
}

void stopWater()
{
  state.waterState = OFF;
  state.currentTurn = 0;
  for (int i = 0; i < MAX_ZONE_NUMBER; i++)
  {
    if (zoneConfig.zone[i].waterOrder == state.currentTurn)
      state.currentZone = i;
  }
  uint8_t outputValues[] = {B00000000};
  setRelays(outputValues);

  writeConfig();
  if (databaseConnected)
    setWaterStateLog();

  TelnetPrint.print("Stop Water at ");
  TelnetPrint.println(UnixTimestamp);

  led[0] = JLed(LED_PIN).On().DelayAfter(1000);
  led[1] = JLed(LED_PIN).On().DelayAfter(1000);
}

void waterTank()
{
  if (!tankLevel)
  {
    if (!(valveState & 1)) // Tank Valve is OFF
    {
      uint8_t outputValues[] = {B00000000};
      waterTankTimestamp = UnixTimestamp;

      outputValues[0] = 1 | 1 << 7;
      setRelays(outputValues);

      led[0] = JLed(LED_PIN).Blink(100, 100).Repeat(3);
      led[1] = JLed(LED_PIN).Blink(1000, 100).Repeat(3);

      prevState = state.waterState;
      state.waterState = TANK_WATER;
      // writeConfig();
      if (databaseConnected)
        setWaterStateLog();
      TelnetPrint.print("Water Tank at ");
      TelnetPrint.println(UnixTimestamp);

      Serial.println("--------------------");
      Serial.println("Water Tank Start...");
      Serial.println("--------------------");
    }
  }
}

void stopWaterTank()
{
  if (tankLevel)
  {
    uint8_t outputValues[] = {B00000000};
    if (prevState == OFF)
      stopWater();
    else if (prevState == AUTO_WATER || prevState == MANUAL_WATER) //VERRR
    {
      unsigned long waterTankDuration;
      waterTankDuration = UnixTimestamp - waterTankTimestamp;

      Serial.print("Water Tank Duration: ");
      Serial.println(waterTankDuration);

      state.waterState = prevState;
      for (int i = state.currentTurn; i < MAX_ZONE_NUMBER; i++)
      {
        for (int j = 0; j < MAX_ZONE_NUMBER; j++) // Zone
        {
          if (zoneConfig.zone[j].waterOrder == i)
          {
            state.endTimestamp[j] = state.endTimestamp[j] + waterTankDuration;
            if (state.currentTurn == i)
            {
              outputValues[0] = 1 << (j + 1) | 1 << 7; // Turn current turn zone ON
              setRelays(outputValues);

              led[0] = JLed(LED_PIN).Blink(100, 200).Repeat(j + 1);
              led[1] = JLed(LED_PIN).Off().DelayAfter(1000);
            }
            if (i == MAX_ZONE_NUMBER - 1) // Last Turn
              state.currentWaterEndTimestamp = state.endTimestamp[j];

            Serial.print("Water End Time for Zone ");
            Serial.print(j + 1);
            Serial.print(" set to: ");
            Serial.print(state.endTimestamp[j]);
            Serial.println("");
          }
        }
      }
      if (databaseConnected)
        setWaterStateLog();

      TelnetPrint.print("Continue Water at ");
      TelnetPrint.println(UnixTimestamp);
    }
    else if (prevState == SINGLE_WATER) //VERRRR
    {
      stopWater();
    }

    if (alarm2HoldState)
    {
      alarm2HoldState = false;
      alarm2State = true;
    }
    else if (alarm1HoldState)
    {
      alarm1HoldState = false;
      alarm1State = true;

      TelnetPrint.print("alarm1State at ");
      TelnetPrint.println(UnixTimestamp);
    }

    TelnetPrint.print("Tank Full at ");
    TelnetPrint.println(UnixTimestamp);
  }
}

void checkTankLevel()
{
  unsigned long currentMillisCheckTankLevel = millis();
  if (currentMillisCheckTankLevel - previousMillisCheckTankLevel > intervalCheckTankLevel)
  {
    previousMillisCheckTankLevel = currentMillisCheckTankLevel;
    bool reading = digitalRead(TANK_LEVEL_PIN);

    // Serial.print("Tank Level is ");
    // Serial.println(reading);
    // Serial.println("");
    // Serial.println((String)DateTime.hour+":"+(String)DateTime.minute);
    //   Serial.println("UnixTimestamp: "+(String)UnixTimestamp);
    //   Serial.println("UnixTimestamp: "+(String)UnixTimestamp);
    //  Serial.println("soilState: "+(String)state.soilState);
    //     Serial.println("waterState: "+(String)state.waterState);
    //     Serial.println("event: "+(String)event);
    if (reading != prevTankLevel)
    {
      lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
      if (reading != tankLevel)
      {
        tankLevel = reading;
        tankRequest = true;

        Serial.print("Tank Request: Level Change to ");
        Serial.println(tankLevel);
        Serial.println("");
      }
    }
    prevTankLevel = reading;
  }
}

void alarm1Hold()
{

  alarm1HoldState = true;
}

void alarm2Hold()
{
  alarm2HoldState = true;
}

void adminInit()
{
  if (boardMode == 'N')
  {
    boardMode = 'C';
    adminTimeOutCounter = 0;
    WiFi.mode(WIFI_OFF);

    Serial.println("Config Mode Enabled!");
    Serial.print("Setting soft-AP ... ");
    WiFi.mode(WIFI_AP);
    Serial.println(WiFi.softAP(ACCESS_POINT_NAME, ACCESS_POINT_PASSWORD) ? "Ready" : "Failed!");

    server.on("/", []()
              {
                Serial.println("admin.html");
                server.send(200, "text/html", PAGE_AdminMainPage);
              });
    server.on("/admin.html", []()
              {
                Serial.println("admin.html");
                server.send(200, "text/html", PAGE_AdminMainPage);
              });
    server.on("/config.html", send_network_configuration_html);
    server.on("/info.html", []()
              {
                Serial.println("info.html");
                server.send(200, "text/html", PAGE_Information);
              });
    server.on("/ntp.html", send_NTP_configuration_html);
    server.on("/style.css", []()
              {
                Serial.println("style.css");
                server.send(200, "text/css", PAGE_Style_css);
              });
    server.on("/microajax.js", []()
              {
                Serial.println("microajax.js");
                server.send(200, "text/plain", PAGE_microajax_js);
              });
    server.on("/admin/values", send_network_configuration_values_html);
    server.on("/admin/connectionstate", send_connection_state_values_html);
    server.on("/admin/infovalues", send_information_values_html);
    server.on("/admin/ntpvalues", send_NTP_configuration_values_html);
    server.on("/admin/clientinit", send_client_init_html);

    server.onNotFound([]()
                      {
                        Serial.println("Page Not Found");
                        server.send(400, "text/html", "Page not Found");
                      });
    server.begin();
    Serial.println("HTTP server started");

    led[0] = JLed(LED_PIN).Blink(500, 500).Repeat(1);
    led[1] = JLed(LED_PIN).Blink(500, 500).Repeat(1);
  }
  else
  {
    boardMode = 'N';
    led[0] = JLed(LED_PIN).On().DelayAfter(2000);
    led[1] = JLed(LED_PIN).On().DelayAfter(2000);
  }
}

void clientInit()
{
  // WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  WiFi.softAPdisconnect(true);
  Serial.println("Setting Station Mode ... ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(networkConfig.ssid.c_str(), networkConfig.password.c_str());
  Serial.println("Connecting to Wi-Fi");
  blink();

  //Sensor initialization
  if (!soilSensor.begin(0x36))
  {
    Serial.println("ERROR! seesaw not found");
    systemConfig.sensorMode = false;
    error.sensorNotAnswering = true;
    writeConfig();
  }
  else
  {
    Serial.print("seesaw started! version: ");
    Serial.println(soilSensor.getVersion(), HEX);
    // Wire.setClock(10000);
    blink();
    filterCapSensorData();
    blink();
    filterTempSensorData();
    blink();
  }
  WiFiInit();
}