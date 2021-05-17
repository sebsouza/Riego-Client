long lastReconnectAttempt = 0;

void mqttConfig()
{
    client.setServer(mqttServer, mqttPort); // Connect to PubNub.
    client.setCallback(callback);
    lastReconnectAttempt = 0;
    outChannelName = outPrefix + auth.token.uid.c_str();
    inChannelName = inPrefix + auth.token.uid.c_str();
    if (client.connected())
    {
        mqttConnected = true;

        TelnetPrint.print("MQTT Connected at ");
        TelnetPrint.println(UnixTimestamp);
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    DynamicJsonDocument sub(128);
    DeserializationError error = deserializeJson(sub, payload, length);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    if (sub["from"] == "user")
    {
        cmd = sub["cmd"];
        DynamicJsonDocument pub(128);
        pub["from"] = "client";
        switch (cmd)
        {
        case getState:
            pub["cmd"] = "setState";
            setWaterStateLog();
            break;
        case saveSystemConfig:
            getSystemConfig();
            pub["cmd"] = "akgSystemConfig";
            break;
        case saveScheduleConfig:
            getScheduleConfig();
            pub["cmd"] = "akgScheduleConfig";
            break;
        case saveZoneConfig:
            getZoneConfig();
            pub["cmd"] = "akgZoneConfig";
            break;
        case saveAirValue:
            systemConfig.airValue = filterCapSensorData() - 1.5;
            writeConfig();
            setAirValue();
            pub["cmd"] = "akgSaveAirValue";
            break;
        case saveWaterValue:
            systemConfig.waterValue = systemConfig.airValue + (100.0 / (float)systemConfig.wetLimit) * (filterCapSensorData() - systemConfig.airValue) + 1.5;
            writeConfig();
            setWaterValue();
            pub["cmd"] = "akgSaveWaterValue";
            break;
        case resetAirWaterValues:
            systemConfig.airValue = AIR_VALUE;
            systemConfig.waterValue = WATER_VALUE;
            writeConfig();
            setAirValue();
            setWaterValue();
            pub["cmd"] = "akgAirWaterReset";
            break;
        case saveValveState:
            getValveState();
            pub["cmd"] = "akgSaveValveState";
            break;
        case setAdminMode:
            adminInit();
            pub["cmd"] = "akgSetAdminMode";
            break;
        default:
            break;
        }
        char buffer[128];
        size_t n = serializeJson(pub, buffer);
        client.publish(outChannelName.c_str(), buffer, n);
    }
}

// boolean reconnect()
// {
//     if (client.connect(clientID))
//     {
//         client.subscribe(inChannelName.c_str()); // Subscribe.
//     }
//     return client.connected();
// }
void handleMqtt()
{
    mqttConnected = client.connected();

    if (!mqttConnected)
    {
        long now = millis();
        if (now - lastReconnectAttempt > 5000)
        { // Try to reconnect.
            lastReconnectAttempt = now;

            Serial.println("Trying to reconnect MQTT...");
            TelnetPrint.print("Trying to reconnect MQTT at ");
            TelnetPrint.println(UnixTimestamp);

            client.disconnect();
            mqttConnected = client.connect(clientID);

            if (mqttConnected)
            { // Attempt to reconnect.
                lastReconnectAttempt = 0;
                client.subscribe(inChannelName.c_str()); // Subscribe.

                Serial.println("MQTT Connected");
                TelnetPrint.print("MQTT Connected at ");
                TelnetPrint.println(UnixTimestamp);
            }
        }
    }
    else
    { // Connected.
        client.loop();
    }
}