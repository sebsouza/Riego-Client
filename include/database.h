#include "global.h"

void setWarningAlarms();

void databaseConfig()
{

    fbconfig.host = FIREBASE_HOST;
    fbconfig.api_key = API_KEY;
    auth.user.email = networkConfig.fbEmail.c_str();
    auth.user.password = networkConfig.fbPassword.c_str();
    Firebase.begin(&fbconfig, &auth);
    Firebase.reconnectWiFi(true);
    Firebase.setFloatDigits(2);
    Firebase.setDoubleDigits(6);
    // fbdo1.setResponseSize(2048);

    Serial.println("Connecting to Firebase");

    int retries = WIFI_CONN_MAX_RETRIES;
    while (Firebase.authTokenInfo().status != token_status_ready && retries-- > 0)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    if (retries > 0)
        databaseConnected = true;
    else
        databaseConnected = false;

    if (databaseConnected)
    {
        Serial.print("Connected with UID: ");
        Serial.println(auth.token.uid.c_str());
        Serial.println();

        path = basePath + auth.token.uid.c_str();

        systemConfigPath = path + "/config/system";
        scheduleConfigPath = path + "/config/schedule";
        // zoneConfigPath = path + "/config/zone";
        _zoneConfigPath = path + "/zones/zone";
        valveSetPath = path + "/valveSet/valveState";
        endTimesPath = path + "/endTimesLog/";
        measuresPath = path + "/measures/";
        waterStateLogPath = path + "/waterState/";
        warningsLogPath = path + "/warnings/";

        Serial.println("Firebase Config Finished.");
        Serial.println();
    }
    else
    {
        Serial.println("Firebase NOT Config !");
        Serial.println();
    }
}

void reconnectDatabase()
{
    long currentMillis = millis();
    if (currentMillis - previousMillisDatabaseReconnect > intervalDatabseReconnect)
    { // Try to reconnect.
        previousMillisDatabaseReconnect = currentMillis;

        Serial.println("Trying to reconnect Database...");
        TelnetPrint.print("Trying to reconnect Database at ");
        TelnetPrint.println(UnixTimestamp);

        databaseConfig();

        if (databaseConnected)
        {
            Serial.println("Database Connected");
            TelnetPrint.print("Database Connected at ");
            TelnetPrint.println(UnixTimestamp);
        }
    }
}

void setWarningAlarms()
{
    Serial.println("------------------------------------");
    Serial.println("Set Warning Alarm JSON...");
    Serial.println("------------------------------------");

    String content;
    DynamicJsonDocument doc(256);
    doc["fields"]["sensorNotAnswering"]["booleanValue"] = bool(error.sensorNotAnswering);
    doc["fields"]["sensorOutOfRange"]["booleanValue"] = bool(error.sensorOutOfRange);
    doc["fields"]["timestamp"]["integerValue"] = UnixTimestamp;
    serializeJson(doc, content);
    if (Firebase.Firestore.createDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, warningsLogPath.c_str(), content.c_str()))
    {
        Serial.println("Warning Alarm PASSED");
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void setMeasures()
{
    if (!error.sensorNotAnswering && !error.sensorOutOfRange)
    {
        unsigned long currentMillisSetMeasure = millis();
        if (currentMillisSetMeasure - previousMillisSetMeasures > intervalSetMeasures)
        {
            previousMillisSetMeasures = currentMillisSetMeasure;
            Serial.println("------------------------------------");
            Serial.println("Set Measures JSON...");
            Serial.println("------------------------------------");

            Serial.println("");
            Serial.print("Measured Sensor Output Value = ");
            Serial.print(measuredSoilSensorValue);
            Serial.print(" / Estimated Sensor Output Value = ");
            Serial.println(estimatedSoilSensorValue);

            TelnetPrint.print("Measured Sensor Output Value = ");
            TelnetPrint.print(measuredSoilSensorValue);
            TelnetPrint.print(" / Estimated Sensor Output Value = ");
            TelnetPrint.println(estimatedSoilSensorValue);

            String content;
            DynamicJsonDocument doc(256);
            doc["fields"]["soilMoisture"]["doubleValue"] = soilMoisture;
            doc["fields"]["estimatedTemperature"]["doubleValue"] = estimatedTemperature;
            doc["fields"]["timestamp"]["integerValue"] = UnixTimestamp;

            serializeJson(doc, content);

            if (Firebase.Firestore.createDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, measuresPath.c_str(), content.c_str()))
            {
                Serial.println("Measures PASSED");
                // Serial.println("------------------------------------");
                // Serial.println(fbdo1.payload());
                Serial.println("------------------------------------");
                Serial.println();
            }
            else
            {
                Serial.println("FAILED");
                Serial.println("REASON: " + fbdo1.errorReason());
                Serial.println("------------------------------------");
                Serial.println();
            }
        }
    }
}

void setWaterAutoInit()
{
    Serial.println("------------------------------------");
    Serial.println("Set Water Auto Init Measure...");
    Serial.println("------------------------------------");

    Serial.println("");
    Serial.print("Measured Sensor Output Value = ");
    Serial.print(measuredSoilSensorValue);
    Serial.print(" / Estimated Sensor Output Value = ");
    Serial.println(estimatedSoilSensorValue);

    TelnetPrint.print("Measured Sensor Output Value = ");
    TelnetPrint.print(measuredSoilSensorValue);
    TelnetPrint.print(" / Estimated Sensor Output Value = ");
    TelnetPrint.println(estimatedSoilSensorValue);

    String content;
    DynamicJsonDocument doc(256);
    doc["fields"]["soilMoistInit"]["doubleValue"] = state.soilMoistInit;
    doc["fields"]["timestamp"]["integerValue"] = UnixTimestamp;

    serializeJson(doc, content);

    if (Firebase.Firestore.createDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, measuresPath.c_str(), content.c_str()))
    {
        Serial.println("Water Auto Init Measure PASSED");
        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println("------------------------------------");
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void setWaterStateLog()
{
    Serial.println("------------------------------------");
    Serial.println("Set Water State Log JSON...");
    Serial.println("------------------------------------");

    String content;
    DynamicJsonDocument doc(1024);

    for (int i = 0; i < MAX_ZONE_NUMBER; i++)
    {
        doc["fields"]["valveState"]["mapValue"]["fields"]["zone" + (String)(i + 1)]["booleanValue"] = bool(valveState & (1 << (i + 1)));
    }
    doc["fields"]["state"]["integerValue"] = state.waterState;
    switch (state.waterState)
    {
    case AUTO_WATER:
        // doc["fields"]["waterControl"]["integerValue"] = int(state.waterControl);
        doc["fields"]["currentWaterZone"]["integerValue"] = int(state.currentZone);
        doc["fields"]["currentWaterStartTimestamp"]["integerValue"] = state.currentWaterStartTimestamp;
        doc["fields"]["currentWaterEndTimestamp"]["integerValue"] = state.currentWaterEndTimestamp;
        doc["fields"]["soilMoistInit"]["doubleValue"] = state.soilMoistInit;
        for (int i = 0; i < MAX_ZONE_NUMBER; i++)
        {
            doc["fields"]["endTimes"]["mapValue"]["fields"]["zone" + (String)(i + 1)]["integerValue"] = state.endTimestamp[i];
        }
        break;
    case MANUAL_WATER:
        doc["fields"]["currentWaterZone"]["integerValue"] = int(state.currentZone);
        doc["fields"]["currentWaterStartTimestamp"]["integerValue"] = state.currentWaterStartTimestamp;
        doc["fields"]["currentWaterEndTimestamp"]["integerValue"] = state.currentWaterEndTimestamp;
        doc["fields"]["soilMoistInit"]["doubleValue"] = state.soilMoistInit;
        for (int i = 0; i < MAX_ZONE_NUMBER; i++)
        {
            doc["fields"]["endTimes"]["mapValue"]["fields"]["zone" + (String)(i + 1)]["integerValue"] = state.endTimestamp[i];
        }
        break;
    case TANK_WATER:
        doc["fields"]["valveState"]["mapValue"]["fields"]["tank"]["booleanValue"] = bool(valveState & 1);
    default:
        doc["fields"]["soilMoisture"]["doubleValue"] = soilMoisture;
        doc["fields"]["estimatedTemperature"]["doubleValue"] = estimatedTemperature;
        break;
    }
    doc["fields"]["timestamp"]["integerValue"] = UnixTimestamp;
    serializeJson(doc, content);

    if (Firebase.Firestore.createDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, waterStateLogPath.c_str(), content.c_str()))
    {
        Serial.println("Water State Log PASSED");
        Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void setEndTimes()
{
    Serial.println("------------------------------------");
    Serial.println("Set End Times...");
    Serial.println("------------------------------------");

    String content;
    DynamicJsonDocument doc(768);
    for (int i = 0; i < MAX_ZONE_NUMBER; i++)
    {
        doc["fields"]["zone" + (String)(i + 1)]["integerValue"] = state.endTimestamp[i];
    }

    serializeJson(doc, content);

    if (Firebase.Firestore.patchDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, endTimesPath.c_str(), content.c_str(), "" /* updateMask */))
    {
        Serial.println("End Times PASSED");
        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println("------------------------------------");
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void setAirValue()
{
    Serial.println("------------------------------------");
    Serial.println("Set Air Value...");
    Serial.println("------------------------------------");

    String content;
    DynamicJsonDocument doc(768);

    doc["fields"]["autoMode"]["booleanValue"] = bool(systemConfig.autoMode);
    doc["fields"]["sensorMode"]["booleanValue"] = bool(systemConfig.sensorMode);
    doc["fields"]["waterValue"]["integerValue"] = systemConfig.waterValue;
    doc["fields"]["airValue"]["integerValue"] = systemConfig.airValue;
    doc["fields"]["dryLimit"]["integerValue"] = systemConfig.dryLimit;
    doc["fields"]["wetLimit"]["integerValue"] = systemConfig.wetLimit;

    serializeJson(doc, content);

    if (Firebase.Firestore.patchDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, systemConfigPath.c_str(), content.c_str(), "" /* updateMask */))
    {
        Serial.println("Air Value PASSED");
        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println("------------------------------------");
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void setWaterValue()
{
    Serial.println("------------------------------------");
    Serial.println("Set Water Value...");
    Serial.println("------------------------------------");

    String content;
    DynamicJsonDocument doc(768);

    doc["fields"]["autoMode"]["booleanValue"] = bool(systemConfig.autoMode);
    doc["fields"]["sensorMode"]["booleanValue"] = bool(systemConfig.sensorMode);
    doc["fields"]["waterValue"]["integerValue"] = systemConfig.waterValue;
    doc["fields"]["airValue"]["integerValue"] = systemConfig.airValue;
    doc["fields"]["dryLimit"]["integerValue"] = systemConfig.dryLimit;
    doc["fields"]["wetLimit"]["integerValue"] = systemConfig.wetLimit;

    serializeJson(doc, content);

    if (Firebase.Firestore.patchDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, systemConfigPath.c_str(), content.c_str(), "" /* updateMask */))
    {
        Serial.println("Water Value PASSED");
        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println("------------------------------------");
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void resetValveState()
{
    Serial.println("------------------------------------");
    Serial.println("Reset Valve State...");
    Serial.println("------------------------------------");

    String content;
    DynamicJsonDocument doc(768);

    for (int i = 0; i < MAX_ZONE_NUMBER; i++)
    {
        doc["fields"]["zone" + (String)(i + 1)]["booleanValue"] = false;
    }
    serializeJson(doc, content);
    if (Firebase.Firestore.patchDocument(&fbdo1, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, valveSetPath.c_str(), content.c_str(), "" /* updateMask */))
    {
        Serial.println("Valve State Reset PASSED");
        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println("------------------------------------");
        Serial.println();
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void getSystemConfig()
{
    Serial.println("------------------------------------");
    Serial.println("Get System Config JSON...");
    Serial.println("------------------------------------");

    if (Firebase.Firestore.getDocument(&fbdo1, FIREBASE_PROJECT_ID, "", systemConfigPath.c_str()))
    {

        String json = fbdo1.payload();
        DynamicJsonDocument doc(662);
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        systemConfig.autoMode = doc["fields"]["autoMode"]["booleanValue"];
        systemConfig.sensorMode = doc["fields"]["sensorMode"]["booleanValue"];
        systemConfig.waterValue = doc["fields"]["waterValue"]["integerValue"];
        systemConfig.airValue = doc["fields"]["airValue"]["integerValue"];
        systemConfig.dryLimit = doc["fields"]["dryLimit"]["integerValue"];
        systemConfig.wetLimit = doc["fields"]["wetLimit"]["integerValue"];
        systemConfig.sensorZone = doc["fields"]["sensorZone"]["integerValue"];

        writeConfig();

        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println();
        Serial.println("\\\\\\ System Config RECIEVED & SAVED //////");
        Serial.println("------------------------------------");
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
    // fbdo1.stopWiFiClient();
}

void getScheduleConfig()
{
    Serial.println("------------------------------------");
    Serial.println("Get Schedule Config JSON...");
    Serial.println("------------------------------------");

    if (Firebase.Firestore.getDocument(&fbdo1, FIREBASE_PROJECT_ID, "", scheduleConfigPath.c_str()))
    {
        String json = fbdo1.payload();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        scheduleConfig.water1StartHour = doc["fields"]["water1StartHour"]["integerValue"];
        scheduleConfig.water1StartMinute = doc["fields"]["water1StartMinute"]["integerValue"];
        scheduleConfig.water2StartHour = doc["fields"]["water2StartHour"]["integerValue"];
        scheduleConfig.water2StartMinute = doc["fields"]["water2StartMinute"]["integerValue"];
        scheduleConfig.wdayOn = 0;
        for (int i = 0; i < 7; i++)
        {
            scheduleConfig.wdayOn |= bool(doc["fields"]["wdayOn"]["mapValue"]["fields"][(String)i]["booleanValue"]) << i;
        }
        writeConfig();

        Serial.println("{{{{{{ Schedule Config RECIEVED 6 SAVED }}}}}}");
        // Serial.println(scheduleConfig.wdayOn, BIN);
        Serial.println("------------------------------------");
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}

void getZoneConfig()
{
    Serial.println("------------------------------------");
    Serial.println("Get Zone Config JSON...");
    Serial.println("------------------------------------");

    for (int i = 0; i < MAX_ZONE_NUMBER; i++)
    {
        String zonePath = _zoneConfigPath + String(i + 1);
        if (Firebase.Firestore.getDocument(&fbdo1, FIREBASE_PROJECT_ID, "", zonePath.c_str(), "waterAuto,waterCapacity,waterOrder,waterQ,waterQMax"))
        {
            String json = fbdo1.payload();
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, json);
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }
            zoneConfig.zone[i].waterAuto = doc["fields"]["waterAuto"]["booleanValue"];
            zoneConfig.zone[i].waterCapacity = doc["fields"]["waterCapacity"]["integerValue"];
            zoneConfig.zone[i].waterOrder = doc["fields"]["waterOrder"]["integerValue"];
            zoneConfig.zone[i].waterQ = doc["fields"]["waterQ"]["integerValue"];
            zoneConfig.zone[i].waterQMax = doc["fields"]["waterQMax"]["integerValue"];

            writeConfig();

            // Serial.println("------------------------------------");
            // Serial.println(fbdo1.payload());
            Serial.println("$$$ Zone " + String(i + 1) + " Config RECIEVED & SAVED $$$");
            Serial.println("------------------------------------");
        }
        else
        {
            Serial.println("! Zone Config FAILED !");
            Serial.println("REASON: " + fbdo1.errorReason());
            Serial.println("------------------------------------");
            Serial.println();
        }
    }
}

void getValveState()
{
    Serial.println("------------------------------------");
    Serial.println("Get Valve State...");
    Serial.println("------------------------------------");

    if (Firebase.Firestore.getDocument(&fbdo1, FIREBASE_PROJECT_ID, "", valveSetPath.c_str()))
    {
        String json = fbdo1.payload();
        DynamicJsonDocument doc(724);
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return;
        }
        newValveState = 0;
        for (int i = 0; i < MAX_ZONE_NUMBER; i++)
        {
            newValveState |= (doc["fields"]["zone" + (String)(i + 1)]["booleanValue"] ? 1 : 0) << i; // 0: Zone1
        }
        appEvent = true;

        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println();
        Serial.println("\\\\\\ System Config RECIEVED & SAVED //////");
        Serial.println("------------------------------------");
    }
    else
    {
        Serial.println("FAILED");
        Serial.println("REASON: " + fbdo1.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
    }
}