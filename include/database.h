#include "global.h"

// void setState();
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

    Serial.println("Connecting to Firebase");

    while (Firebase.authTokenInfo().status != token_status_ready)
    {
        Serial.print(".");
        delay(50);
    }

    Serial.println();
    Serial.print("Connected with UID: ");
    Serial.println(auth.token.uid.c_str());
    Serial.println();

    path = basePath + auth.token.uid.c_str();

    systemConfigPath = path + "/config/system";
    scheduleConfigPath = path + "/config/schedule";
    zoneConfigPath = path + "/config/zone";
    valveSetPath = path + "/valveSet/valveState";
    endTimesPath = path + "/endTimesLog/";
    measuresPath = path + "/measures/";
    waterStateLogPath = path + "/waterState/";
    warningsLogPath = path + "/warnings/";

    Serial.println("Firebase Config Finished.");
    Serial.println();
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
            Serial.print("Free Heap: ");
            Serial.println(ESP.getFreeHeap());
            Serial.println("");

            Serial.print("Soil is ");
            Serial.print((state.soilState == 0   ? "DRY"
                          : state.soilState == 1 ? "WET"
                          : state.soilState == 2 ? "VERY WET"
                                                 : "UNKNOWN"));
            Serial.print(": RH = ");
            Serial.print(soilMoisture);
            Serial.print(" %; Estimated Sensor Output Value = ");
            Serial.println(estimatedSoilSensorValue);
            Serial.print("Measured Temperature: ");
            Serial.print(measuredTemperature);
            Serial.print("; Estimated Temperature: ");
            Serial.println(estimatedTemperature);
            Serial.println("");

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

    doc["fields"]["airValue"]["integerValue"] = systemConfig.airValue;

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

    doc["fields"]["waterValue"]["integerValue"] = systemConfig.waterValue;

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

    if (Firebase.Firestore.getDocument(&fbdo1, FIREBASE_PROJECT_ID, "", zoneConfigPath.c_str()))
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
        for (int i = 0; i < MAX_ZONE_NUMBER; i++)
        {
            zoneConfig.zone[i].waterAuto = doc["fields"]["zone" + (String)(i + 1)]["mapValue"]["fields"]["waterAuto"]["booleanValue"];
            zoneConfig.zone[i].waterCapacity = doc["fields"]["zone" + (String)(i + 1)]["mapValue"]["fields"]["waterCapacity"]["integerValue"];
            zoneConfig.zone[i].waterOrder = doc["fields"]["zone" + (String)(i + 1)]["mapValue"]["fields"]["waterOrder"]["integerValue"];
            zoneConfig.zone[i].waterQ = doc["fields"]["zone" + (String)(i + 1)]["mapValue"]["fields"]["waterQ"]["integerValue"];
            // zoneConfig.zone[i].name = urldecode(doc["fields"]["zone" + (String)(i + 1)]["mapValue"]["fields"]["name"]["stringValue"]);
            zoneConfig.zone[i].waterQMax = doc["fields"]["zone" + (String)(i + 1)]["mapValue"]["fields"]["waterQMax"]["integerValue"];
        }

        writeConfig();

        // Serial.println("------------------------------------");
        // Serial.println(fbdo1.payload());
        Serial.println("$$$$$ Zone Config RECIEVED & SAVED $$$$$$");
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