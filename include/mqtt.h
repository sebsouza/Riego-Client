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
        char buffer[128];
        size_t n;
        cmd = sub["cmd"];
        DynamicJsonDocument pub(128);
        pub["from"] = "client";
        switch (cmd)
        {
        case getState: // 0
            pub["cmd"] = "setState";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            previousMillisGetMeasures = millis() + intervalGetMeasures + 1;
            getMeasures();
            setWaterStateLog();
            previousMillisSetMeasures = millis() + intervalSetMeasures + 1;
            break;
        case saveSystemConfig:
            pub["cmd"] = "akgSystemConfig";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            getSystemConfig();
            break;
        case saveScheduleConfig:
            pub["cmd"] = "akgScheduleConfig";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            getScheduleConfig();
            break;
        case saveZoneConfig:
            pub["cmd"] = "akgZoneConfig";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            getZoneConfig();
            break;
        case saveAirValue: // 4
            pub["cmd"] = "akgSaveAirValue";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            systemConfig.airValue = filterCapSensorData() - 1.5;
            writeConfig();
            setAirValue();
            previousMillisGetMeasures = millis() + intervalGetMeasures + 1;
            getMeasures();
            setWaterStateLog();
            previousMillisSetMeasures = millis() + intervalSetMeasures + 1;

            Serial.print("Air Value set to: ");
            Serial.println(systemConfig.airValue);
            TelnetPrint.print("Air Value set to: ");
            TelnetPrint.println(systemConfig.airValue);
            break;
        case saveWaterValue:
            // systemConfig.waterValue = systemConfig.airValue + (100.0 / (float)systemConfig.wetLimit) * (filterCapSensorData() - systemConfig.airValue) + 1.5;
            pub["cmd"] = "akgSaveWaterValue";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            systemConfig.waterValue = filterCapSensorData() + 1.5;
            writeConfig();
            setWaterValue();
            previousMillisGetMeasures = millis() + intervalGetMeasures + 1;
            getMeasures();
            setWaterStateLog();
            previousMillisSetMeasures = millis() + intervalSetMeasures + 1;

            Serial.print("Water Value set to: ");
            Serial.println(systemConfig.waterValue);
            TelnetPrint.print("Water Value set to: ");
            TelnetPrint.println(systemConfig.waterValue);
            break;
        case resetAirWaterValues:
            pub["cmd"] = "akgAirWaterReset";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            systemConfig.airValue = AIR_VALUE;
            systemConfig.waterValue = WATER_VALUE;
            writeConfig();
            setAirValue();
            setWaterValue();
            previousMillisGetMeasures = millis() + intervalGetMeasures + 1;
            getMeasures();
            setWaterStateLog();
            previousMillisSetMeasures = millis() + intervalSetMeasures + 1;

            Serial.print("Air Value set to: ");
            Serial.println(systemConfig.airValue);
            TelnetPrint.print("Air Value set to: ");
            TelnetPrint.println(systemConfig.airValue);
            Serial.print("Water Value set to: ");
            Serial.println(systemConfig.waterValue);
            TelnetPrint.print("Water Value set to: ");
            TelnetPrint.println(systemConfig.waterValue);
            break;
        case saveValveState:
            pub["cmd"] = "akgSaveValveState";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            getValveState();
            break;
        case setAdminMode: // 8
            pub["cmd"] = "akgSetAdminMode";
             n = serializeJson(pub, buffer);
            client.publish(outChannelName.c_str(), buffer, n);
             handleMqtt();
            adminInit();
            break;
        default:
            break;
        }

        // size_t n = serializeJson(pub, buffer);
        // client.publish(outChannelName.c_str(), buffer, n);
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
