#ifndef GLOBAL_H
#define GLOBAL_H

#define ACCESS_POINT_NAME "RiegoAdmin"
#define ACCESS_POINT_PASSWORD "12345678"
#define ADMIN_TIMEOUT 180 // Defines the Time in Seconds, when the Admin-Mode will be diabled

#define WIFI_CONN_MAX_RETRIES 20

#define AIR_VALUE 600
#define WATER_VALUE 1015

#define MAX_ZONE_NUMBER 4

constexpr auto MIN_WATERQ = 0.5; // mm/m2

constexpr auto BUTTON_PIN = 19;
constexpr auto TANK_LEVEL_PIN = 18;

#define RELAY_SERIAL_PIN 25
#define RELAY_OUTPUT_ENABLE 26
#define RELAY_LATCH_PIN 27
#define RELAY_CLOCK_PIN 14

constexpr auto LED_PIN = 12;

bool tankRequest = false,
	 appEvent = false,
	 buttonSingle = false,
	 buttonLong = false,
	 alarm1State = false,
	 alarm2State = false,
	 alarm1HoldState = false,
	 alarm2HoldState = false,
	 dbInit = false;

unsigned long intervalGetMeasures = 1000 * 60 * 1, // // Interval at which to read sensors: 1 minute
	previousMillisGetMeasures = 0;

unsigned long intervalSetMeasures = 1000 * 60 * 10; // Update history every 60 minutes
unsigned long previousMillisSetMeasures = 0;

unsigned long intervalCheckTankLevel = 1000 * 1; // Check Tank Level every 1 second
unsigned long previousMillisCheckTankLevel = 0;	 // Time of last point added

unsigned long intervalWifiReconnect = 30000;
unsigned long previousMillisWifiReconnect = 0;

unsigned long intervalDatabseReconnect = 5000;
unsigned long previousMillisDatabaseReconnect = 0;

unsigned long waterTankTimestamp;
bool tankLevel = true,
	 prevTankLevel = true;
unsigned long debounceDelay = 50; // Switch debounce in milliseconds
unsigned long lastDebounceTime = 0;

uint16_t measuredSoilSensorValue;
float estimatedSoilSensorValue;
float soilMoisture = 0;
float measuredTemperature, estimatedTemperature;

// int soilMoistureLastError = 0;
float soilMoistureErrorIntegration[MAX_ZONE_NUMBER];
float antiWindup[MAX_ZONE_NUMBER];

// int bootTimes;
char boardMode = 'N'; // Normal operation or Configuration mode?
bool WiFiConnected = false;

WebServer server(80);			 // The Webserver
boolean firstStart = true;		 // On firststart = true, NTP will try to get a valid time
int adminTimeOutCounter = 0;	 // Counter for Disabling the AdminMode
strDateTime DateTime;			 // Global DateTime structure, will be refreshed every Second
WiFiUDP UDPNTPClient;			 // NTP Client
unsigned long UnixTimestamp = 0; // GLOBALTIME  ( Will be set by NTP)
int cNTP_Update = 0;			 // Counter for Updating the time via NTP
Ticker tkSecond;				 // Second - Timer for Updating Datetime Structure
// boolean adminEnabled = false; // Enable Admin Mode for a given Time
byte minuteOld = 100; // Helpvariable for checking, when a new Minute comes up (for Auto Turn On / Off)
byte dayOld = 100;
long customWatchdog;

bool databaseConnected = false;

FirebaseData fbdo1;
FirebaseAuth auth;
FirebaseConfig fbconfig;
String path = "";
String base_path = "/UsersData/";
String systemConfig_path = "";
String scheduleConfig_path = "";
String zoneConfig_path = "";
String measures_path = "";
String state_path = "";
String valveHist_path = "";
String valveState_path = "";
String warningAlarms_path = "";

String basePath = "users/";
String systemConfigPath = "";
// String zoneConfigPath = "";
String _zoneConfigPath = "";
String scheduleConfigPath = "";
String valveSetPath = "";
String waterStateLogPath = "";
String warningsLogPath = "";
String endTimesPath = "";
String measuresPath = "";

const char *mqttServer = "mqtt.pndsn.com";
const int mqttPort = 1883;
String outChannelName = "";
String inChannelName = "";
String inPrefix = "in-";
String outPrefix = "out-";
WiFiClient MQTTclient;
PubSubClient client(MQTTclient);
bool mqttConnected = false;

bool otaFlag = false;

enum defSeason
{
	SUMMER,
	AUTUMN,
	WINTER,
	SPRING,
	MAX_SEASONS
} season;

// enum defSoilState
// {
// 	DRY,
// 	WET,
// 	VERY_WET,
// 	MAX_SOIL_STATES
// };

enum defState
{
	OFF,
	AUTO_WATER,
	MANUAL_WATER,
	SINGLE_WATER,
	TANK_WATER,
	MAX_STATES
} prevState;

enum defEvent
{
	NO_EVENT,
	ALARM_1,
	ALARM_2,
	APP_EVENT,
	BUTTON_SINGLE,
	BUTTON_LONG,
	TANK_REQUEST,
	MAX_EVENTS
} event;

enum defEvent getNewEvent();

enum defCmd
{
	getState,
	saveSystemConfig,
	saveScheduleConfig,
	saveZoneConfig,
	saveAirValue,
	saveWaterValue,
	resetAirWaterValues,
	saveValveState,
	setAdminMode
} cmd;

// address 16
struct strNetworkConfig
{
	String ssid;
	String password;
	byte IP[4];
	byte Netmask[4];
	byte Gateway[4];
	boolean dhcp;
	String ntpServerName;
	long Update_Time_Via_NTP_Every;
	long timezone;
	boolean daylight;
	String fbEmail;
	String fbPassword;
} networkConfig;

//address 528
struct strSystemConfig
{
	long airValue;
	long waterValue;
	byte dryLimit;
	byte wetLimit;
	boolean sensorMode;
	boolean autoMode;
	byte sensorZone;
} systemConfig;

// address 1040
struct strSheduleConfig
{
	byte water1StartHour;
	byte water1StartMinute;
	byte water2StartHour;
	byte water2StartMinute;
	byte wdayOn; // Sunday is day LSB

} scheduleConfig;

struct strZone
{
	boolean waterAuto;
	byte waterEndHour;
	byte waterEndMinute;
	byte waterOrder;
	byte waterCapacity; // mm/h
	byte waterQ;		// % Set Point
	byte waterQMax;		// mm
};

// address 1552
struct strZoneConfig
{
	strZone zone[MAX_ZONE_NUMBER];
} zoneConfig;

// address 2064
struct strState
{
	defState waterState;
	// defSoilState soilState;
	byte currentTurn;
	byte currentZone;
	float soilMoistInit;
	unsigned long currentWaterStartTimestamp;
	unsigned long currentWaterEndTimestamp;
	unsigned long endTimestamp[MAX_ZONE_NUMBER];
	float waterControl[MAX_ZONE_NUMBER]; // 0...100 [%]
} state;

struct strError
{
	bool sensorOutOfRange;
	bool sensorNotAnswering;
} error;

uint8_t valveState,
	newValveState = 0;
/*
*	0: ... tank
*	1: ... zone_1
*		...
*	MAX_ZONE_NUMBER: ... zone_MAX_ZONE_NUMBER
*/

SimpleKalmanFilter soilMoistureKF(50, 50, 0.001);
SimpleKalmanFilter temperatureKF(5, 5, 0.01);

Adafruit_seesaw soilSensor;

ShiftRegister74HC595<1> sr(RELAY_SERIAL_PIN, RELAY_CLOCK_PIN, RELAY_LATCH_PIN);

JLed led[] = {
	JLed(LED_PIN).On().DelayAfter(1000),
	JLed(LED_PIN).On().DelayAfter(1000),
};

auto ledSequence = JLedSequence(JLedSequence::eMode::SEQUENCE, led).Forever();

PinButton button(BUTTON_PIN);

void (*const state_table[MAX_SEASONS]/* [MAX_SOIL_STATES] */[MAX_STATES][MAX_EVENTS])(void) = {
	{/* procedures for SUMMER */
	//  {
		 /* procedures for DRY soil */
		 {checkAlarms, waterAuto, waterAuto, waterSingleZone, waterManual, adminInit, waterTank},	   /* procedures for OFF */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	   /* procedures for AUTO_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	   /* procedures for MANUAL_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		   /* procedures for SINGLE_WATER */
		 {checkAlarms, alarm1Hold, alarm2Hold, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for WET soil */
	// 	 {checkAlarms, waterAuto, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	  /* procedures for OFF */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		  /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, alarm1Hold, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for VERY_WET soil */
	// 	 {checkAlarms, doNothing, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	 /* procedures for OFF */
	// 	 {},																						 /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		 /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  }
	 },
	{/* procedures for AUTUMN */
	//  {
		 /* procedures for DRY soil */
		 {checkAlarms, waterAuto, doNothing, waterSingleZone, waterManual, adminInit, waterTank},  /* procedures for OFF */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank}, /* procedures for AUTO_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank}, /* procedures for MANUAL_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},	   /* procedures for SINGLE_WATER */
		 {checkAlarms, alarm1Hold, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank}  /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for WET soil */
	// 	 {checkAlarms, waterAuto, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	  /* procedures for OFF */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		  /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, alarm1Hold, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for VERY_WET soil */
	// 	 {checkAlarms, doNothing, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	 /* procedures for OFF */
	// 	 {},																						 /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		 /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  }
	 },
	{/* procedures for WINTER */
	//  {
		 /* procedures for DRY soil */
		 {checkAlarms, waterAuto, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	  /* procedures for OFF */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for AUTO_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for MANUAL_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		  /* procedures for SINGLE_WATER */
		 {checkAlarms, alarm1Hold, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for WET soil */
	// 	 {checkAlarms, doNothing, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	 /* procedures for OFF */
	// 	 {},																						 /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		 /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for VERY_WET soil */
	// 	 {checkAlarms, doNothing, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	 /* procedures for OFF */
	// 	 {},																						 /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		 /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  }
	 },
	{/* procedures for SPRING */
	//  {
		 /* procedures for DRY soil */
		 {checkAlarms, waterAuto, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	  /* procedures for OFF */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for AUTO_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	  /* procedures for MANUAL_WATER */
		 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		  /* procedures for SINGLE_WATER */
		 {checkAlarms, alarm1Hold, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for WET soil */
	// 	 {checkAlarms, waterAuto, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	 /* procedures for OFF */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		 /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  },
	//  {
	// 	 /* procedures for VERY_WET soil */
	// 	 {checkAlarms, doNothing, doNothing, waterSingleZone, waterManual, adminInit, waterTank},	 /* procedures for OFF */
	// 	 {},																						 /* procedures for AUTO_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, waterNextZone, stopWater, waterTank},	 /* procedures for MANUAL_WATER */
	// 	 {checkTimer, doNothing, doNothing, waterSingleZone, stopWater, stopWater, waterTank},		 /* procedures for SINGLE_WATER */
	// 	 {checkAlarms, doNothing, doNothing, doNothing, stopWaterTank, stopWaterTank, stopWaterTank} /* procedures for TANK_WATER */
	//  }
	 }};

void writeConfig()
{
	EEPROM.write(0, 'C');
	EEPROM.write(1, 'F');
	EEPROM.write(2, 'G');

	// Network Config (16)
	WriteStringToEEPROM(16, networkConfig.ssid); // 32 byte
	WriteStringToEEPROM(48, networkConfig.password);
	EEPROM.write(80, networkConfig.IP[0]);
	EEPROM.write(81, networkConfig.IP[1]);
	EEPROM.write(82, networkConfig.IP[2]);
	EEPROM.write(83, networkConfig.IP[3]);
	EEPROM.write(84, networkConfig.Netmask[0]);
	EEPROM.write(85, networkConfig.Netmask[1]);
	EEPROM.write(86, networkConfig.Netmask[2]);
	EEPROM.write(87, networkConfig.Netmask[3]);
	EEPROM.write(88, networkConfig.Gateway[0]);
	EEPROM.write(89, networkConfig.Gateway[1]);
	EEPROM.write(90, networkConfig.Gateway[2]);
	EEPROM.write(91, networkConfig.Gateway[3]);
	EEPROM.write(92, networkConfig.dhcp);
	WriteStringToEEPROM(93, networkConfig.ntpServerName);
	EEPROMWritelong(125, networkConfig.Update_Time_Via_NTP_Every); // 4 Byte
	EEPROMWritelong(129, networkConfig.timezone);				   // 4 Byte
	EEPROM.write(133, networkConfig.daylight);
	WriteStringToEEPROM(134, networkConfig.fbEmail);
	WriteStringToEEPROM(166, networkConfig.fbPassword);

	// System Config (272)
	EEPROMWritelong(272, systemConfig.airValue);
	EEPROMWritelong(276, systemConfig.waterValue);
	EEPROM.write(280, systemConfig.dryLimit);
	EEPROM.write(281, systemConfig.wetLimit);
	EEPROM.write(282, systemConfig.sensorMode);
	EEPROM.write(283, systemConfig.autoMode);

	//Schedule Config (336)
	EEPROM.write(336, scheduleConfig.water1StartHour);
	EEPROM.write(337, scheduleConfig.water1StartMinute);
	EEPROM.write(338, scheduleConfig.water2StartHour);
	EEPROM.write(339, scheduleConfig.water2StartMinute);
	EEPROM.write(340, scheduleConfig.wdayOn);

	//Zone Config (400)
	for (int i = 0; i < MAX_ZONE_NUMBER; i++)
	{
		EEPROM.write(400 + i * 64, zoneConfig.zone[i].waterAuto);
		// EEPROM.write(401 + i * 64, zoneConfig.zone[i].waterMaxHour);
		// EEPROM.write(402 + i * 64, zoneConfig.zone[i].waterMaxMinute);
		// EEPROM.write(403 + i * 64, zoneConfig.zone[i].waterEndHour);
		// EEPROM.write(404 + i * 64, zoneConfig.zone[i].waterEndMinute);
		// EEPROM.write(405 + i * 64, zoneConfig.zone[i].manualWaterDurationHour);
		// EEPROM.write(406 + i * 64, zoneConfig.zone[i].manualWaterDurationMinute);
		EEPROM.write(407 + i * 64, zoneConfig.zone[i].waterOrder);
		EEPROM.write(408 + i * 64, zoneConfig.zone[i].waterCapacity);
		EEPROM.write(409 + i * 64, zoneConfig.zone[i].waterQ);
		// WriteStringToEEPROM(410 + i * 64, zoneConfig.zone[i].name);
		EEPROM.write(442 + i * 64, zoneConfig.zone[i].waterQMax);
	}

	// State (912)
	// EEPROM.write(912, state.waterState);
	// EEPROM.write(913, state.soilState);
	// EEPROM.write(914, state.currentTurn);
	// EEPROM.write(915, state.currentZone);
	// EEPROMWriteFloat(916, state.soilMoistInit);
	// EEPROMWritelong(920, state.currentWaterStartTimestamp);
	// EEPROMWritelong(924, state.currentWaterEndTimestamp);
	// for (int i = 0; i < MAX_ZONE_NUMBER; i++)
	// {

	// 	EEPROMWritelong(928 + 4 * i, state.endTimestamp[i]);
	// }

	EEPROM.commit();
}

boolean readConfig()
{

	Serial.println("Reading Configuration");
	if (EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F' && EEPROM.read(2) == 'G')
	{
		Serial.println("Configurarion Found!");

		// Network Config (16)
		networkConfig.ssid = ReadStringFromEEPROM(16);
		networkConfig.password = ReadStringFromEEPROM(48);
		networkConfig.IP[0] = EEPROM.read(80);
		networkConfig.IP[1] = EEPROM.read(81);
		networkConfig.IP[2] = EEPROM.read(82);
		networkConfig.IP[3] = EEPROM.read(83);
		networkConfig.Netmask[0] = EEPROM.read(84);
		networkConfig.Netmask[1] = EEPROM.read(85);
		networkConfig.Netmask[2] = EEPROM.read(86);
		networkConfig.Netmask[3] = EEPROM.read(87);
		networkConfig.Gateway[0] = EEPROM.read(88);
		networkConfig.Gateway[1] = EEPROM.read(89);
		networkConfig.Gateway[2] = EEPROM.read(90);
		networkConfig.Gateway[3] = EEPROM.read(91);
		networkConfig.dhcp = EEPROM.read(92);
		networkConfig.ntpServerName = ReadStringFromEEPROM(93);
		networkConfig.Update_Time_Via_NTP_Every = EEPROMReadlong(125); // 4 Byte
		networkConfig.timezone = EEPROMReadlong(129);				   // 4 Byte
		networkConfig.daylight = EEPROM.read(133);
		networkConfig.fbEmail = ReadStringFromEEPROM(134);
		networkConfig.fbPassword = ReadStringFromEEPROM(166);

		// System Config (272)
		systemConfig.airValue = EEPROMReadlong(272);
		systemConfig.waterValue = EEPROMReadlong(276);
		systemConfig.dryLimit = EEPROM.read(280);
		systemConfig.wetLimit = EEPROM.read(281);
		systemConfig.sensorMode = EEPROM.read(282);
		systemConfig.autoMode = EEPROM.read(283);

		//Schedule Config (336)
		scheduleConfig.water1StartHour = EEPROM.read(336);
		scheduleConfig.water1StartHour = EEPROM.read(337);
		scheduleConfig.water1StartHour = EEPROM.read(338);
		scheduleConfig.water1StartHour = EEPROM.read(339);
		scheduleConfig.wdayOn = EEPROM.read(340);

		//Zone Config (400)
		for (int i = 0; i < MAX_ZONE_NUMBER; i++)
		{
			zoneConfig.zone[i].waterAuto = EEPROM.read(400 + i * 64);
			// zoneConfig.zone[i].waterMaxHour = EEPROM.read(401 + i * 64);
			// zoneConfig.zone[i].waterMaxMinute = EEPROM.read(402 + i * 64);
			// zoneConfig.zone[i].waterEndHour = EEPROM.read(403 + i * 64);
			// zoneConfig.zone[i].waterEndMinute = EEPROM.read(404 + i * 64);
			// zoneConfig.zone[i].manualWaterDurationHour = EEPROM.read(405 + i * 64);
			// zoneConfig.zone[i].manualWaterDurationMinute = EEPROM.read(406 + i * 64);
			zoneConfig.zone[i].waterOrder = EEPROM.read(407 + i * 64);
			zoneConfig.zone[i].waterCapacity = EEPROM.read(408 + i * 64);
			zoneConfig.zone[i].waterQ = EEPROM.read(409 + i * 64);
			// zoneConfig.zone[i].name = ReadStringFromEEPROM(410 + i * 64);
			zoneConfig.zone[i].waterQMax = EEPROM.read(442 + i * 64);
		}

		// State (912)
		// state.waterState = (defState)EEPROM.read(912);
		// state.soilState = (defSoilState)EEPROM.read(913);
		// state.currentTurn = EEPROM.read(914);
		// state.currentZone = EEPROM.read(915);
		// state.soilMoistInit = EEPROMReadFloat(916);
		// state.currentWaterStartTimestamp = EEPROMReadlong(920);
		// state.currentWaterEndTimestamp = EEPROMReadlong(924);
		// for (int i = 0; i < MAX_ZONE_NUMBER; i++)
		// {
		// 	state.endTimestamp[i] = EEPROMReadlong(928 + 4 * i);
		// }

		return true;
	}
	else
	{
		Serial.println("Configuration NOT FOUND!!!!");
		return false;
	}
}

void WiFiInit()
{
	// unsigned long WiFiConnectionStart = millis();

	int retries = WIFI_CONN_MAX_RETRIES;
	while (WiFi.status() != WL_CONNECTED && retries-- > 0)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");

	if (retries > 0)
		WiFiConnected = true;
	else
		WiFiConnected = false;

	if (WiFiConnected)
	{
		//TelnetPrint = NetServer(2323); // uncomment to change port
		TelnetPrint.begin();
		Serial.println("Telnet started");
		blink();

		UDPNTPClient.begin(2390); // Port for NTP receive
		Serial.println("WiFi Init");
		blink();

		databaseConfig();
		blink();
		mqttConfig();
		blink();
		getScheduleConfig();
		blink();
		getSystemConfig();
		blink();
		getZoneConfig();

		for (int i = 0; i < 3; i++)
		{
			JLed(LED_PIN).On().Update();
			delay(500);
			JLed(LED_PIN).Off().Update();
			delay(500);
		}

		led[0] = JLed(LED_PIN).On().DelayAfter(1000);
		led[1] = JLed(LED_PIN).On().DelayAfter(1000);
	}
	else
	{
		Serial.println("WiFi Not Connected!");

		led[0] = JLed(LED_PIN).Blink(150, 150).Repeat(2);
		led[1] = JLed(LED_PIN).Blink(1000, 1000).Repeat(2);
	}
}

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
{
	switch (event)
	{
	case SYSTEM_EVENT_STA_CONNECTED:
		break;
		Serial.println("Connected to AP successfully!");
	case SYSTEM_EVENT_STA_GOT_IP:
		Serial.print("WiFi connected! IP address: ");
		Serial.println(WiFi.localIP());

		WiFiConnected = true;

		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		Serial.println("Disconnected from WiFi access point");
		Serial.print("WiFi lost connection. Reason: ");
		Serial.println(info.disconnected.reason);

		WiFiConnected = false;

		// if (info.disconnected.reason == 201)
		// {
		// 	unsigned long currentMillisWiFiReconnect = millis();
		// 	if (currentMillisWiFiReconnect - previousMillisWifiReconnect > intervalWifiReconnect)
		// 	{
		// 		previousMillisWifiReconnect = currentMillisWiFiReconnect;
		// 		Serial.println("Trying to Reconnect");
		// 		WiFi.mode(WIFI_OFF);
		// 		WiFi.mode(WIFI_STA);
		// 		WiFi.begin(networkConfig.ssid.c_str(), networkConfig.password.c_str());
		// 	}
		// }
		// else if (info.disconnected.reason == 15)
		// {
		// 	adminInit();
		// }
		// break;
	default:
		break;
	}
}

int calculateDayOfYear(byte day, byte month, int year)
{
	int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (year % 4 == 0)
	{
		if (year % 100 != 0)
			daysInMonth[1] = 29;
		else if (year % 400 == 0)
			daysInMonth[1] = 29;
	}
	int doy = 0;
	for (int i = 0; i < month - 1; i++)
		doy += daysInMonth[i];
	doy += day;
	return doy;
}

void getCurrentSeason(int currentDOY, int year)
{
	if (currentDOY < calculateDayOfYear(20, 3, year) || currentDOY >= calculateDayOfYear(21, 12, year))
		season = SUMMER; // SUMMER
	else if (currentDOY >= calculateDayOfYear(21, 3, year) && currentDOY < calculateDayOfYear(20, 6, year))
		season = AUTUMN;
	else if (currentDOY >= calculateDayOfYear(21, 6, year) && currentDOY < calculateDayOfYear(20, 9, year))
		season = WINTER; // WINTER
	else if (currentDOY >= calculateDayOfYear(21, 9, year) && currentDOY < calculateDayOfYear(20, 12, year))
		season = SPRING; // SPRING
}

void checkSeason()
{
	if (DateTime.day != dayOld) // Check for season
	{
		dayOld = DateTime.day;
		Serial.print("Current day is ");
		Serial.println(DateTime.day);
		getCurrentSeason(calculateDayOfYear(DateTime.day, DateTime.month, DateTime.year), DateTime.year);
		Serial.print("Current Season is ");
		Serial.println((season == 0	  ? "SUMMER"
						: season == 1 ? "AUTUMN"
						: season == 2 ? "WINTER"
						: season == 3 ? "SPRING"
									  : "UNKNOWN"));
		Serial.println();
	}
}

void getDateTime()
{
	strDateTime tempDateTime;

	ConvertUnixTimeStamp(UnixTimestamp + (networkConfig.timezone * 360), &tempDateTime);
	if (networkConfig.daylight) // Sommerzeit beachten
		if (summertime(tempDateTime.year, tempDateTime.month, tempDateTime.day, tempDateTime.hour, 0))
			ConvertUnixTimeStamp(UnixTimestamp + (networkConfig.timezone * 360) + 3600, &DateTime);
		else
			DateTime = tempDateTime;
	else
		DateTime = tempDateTime;
}

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
void NTPRefresh()
{
	if (WiFiConnected)
	{
		IPAddress timeServerIP;
		WiFi.hostByName(networkConfig.ntpServerName.c_str(), timeServerIP);
		//sendNTPpacket(timeServerIP); // send an NTP packet to a time server

		Serial.println("sending NTP packet...");

		// TelnetPrint.print("Sending NTP packet at ");
		// TelnetPrint.println(UnixTimestamp);

		memset(packetBuffer, 0, NTP_PACKET_SIZE);
		packetBuffer[0] = 0b11100011; // LI, Version, Mode
		packetBuffer[1] = 0;		  // Stratum, or type of clock
		packetBuffer[2] = 6;		  // Polling Interval
		packetBuffer[3] = 0xEC;		  // Peer Clock Precision
		packetBuffer[12] = 49;
		packetBuffer[13] = 0x4E;
		packetBuffer[14] = 49;
		packetBuffer[15] = 52;
		UDPNTPClient.beginPacket(timeServerIP, 123);
		UDPNTPClient.write(packetBuffer, NTP_PACKET_SIZE);
		UDPNTPClient.endPacket();

		delay(1000);

		int cb = UDPNTPClient.parsePacket();
		if (!cb)
		{
			Serial.println("NTP no packet yet");
			Serial.println();

			TelnetPrint.print("NTP no packet yet at ");
			TelnetPrint.println(UnixTimestamp);
		}
		else
		{
			Serial.print("NTP packet received, length=");
			Serial.println(cb);
			Serial.println();

			TelnetPrint.print("NTP packet received at ");
			TelnetPrint.println(UnixTimestamp);

			UDPNTPClient.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			unsigned long secsSince1900 = highWord << 16 | lowWord;
			const unsigned long seventyYears = 2208988800UL;
			unsigned long epoch = secsSince1900 - seventyYears;
			UnixTimestamp = epoch;

			getDateTime();
			checkSeason();

			if (firstStart)
			{
				setWaterStateLog();
				setWarningAlarms();
				firstStart = false;
				TelnetPrint.print("First Start at ");
				TelnetPrint.println(UnixTimestamp);
			}
		}
	}
	// TelnetPrint.print("NTP refreshed at ");
	// TelnetPrint.println(UnixTimestamp);
}
void getNTP()
{
	if (networkConfig.Update_Time_Via_NTP_Every > 0)
	{
		if (cNTP_Update > 5 && firstStart)
		{
			NTPRefresh();
			cNTP_Update = 0;
			// firstStart = false;
		}
		else if (cNTP_Update > (networkConfig.Update_Time_Via_NTP_Every * 60))
		{
			NTPRefresh();
			cNTP_Update = 0;
		}
	}
}

void Second_Tick()
{
	adminTimeOutCounter++;
	cNTP_Update++;
	UnixTimestamp++;
	// timestamp = UnixTimestamp + (networkConfig.timezone * 360);
	getDateTime();

	// if (event != 0 && event != 1 && event != 2)

	// Serial.print("season: " + (String)season);
	// Serial.print(" soilState: " + (String)state.soilState);
	// Serial.print(" waterState: " + (String)state.waterState);
	// Serial.print(" event: " + (String)event);
	// Serial.println(" @ " + (String)UnixTimestamp);

	TelnetPrint.print("season: " + (String)season);
	// TelnetPrint.print(" soilState: " + (String)state.soilState);
	TelnetPrint.print(" waterState: " + (String)state.waterState);
	TelnetPrint.print(" event: " + (String)event);
	TelnetPrint.println(" @ " + (String)UnixTimestamp);

	if (millis() - customWatchdog > 60000)
	{
		Serial.println("CustomWatchdog bites. Bye");
		TelnetPrint.print("!!!!!!!!!! CustomWatchdog bites at ");
		TelnetPrint.println(UnixTimestamp);
		// ESP.restart();
	}
}

void digitalClockDisplay()
{
	// digital clock display of the time
	Serial.print(DateTime.hour);
	printDigits(DateTime.minute);
	printDigits(DateTime.second);
	Serial.print(" ");
	Serial.print(DateTime.day);
	Serial.print(".");
	Serial.print(DateTime.month);
	Serial.print(".");
	Serial.print(DateTime.year);
	Serial.println();
}

void setRelays(const uint8_t *outputValues)
{
	valveState = outputValues[0];
	const uint8_t digitalValues[] = {outputValues[0] ^ B11110};
	sr.setAll(digitalValues);

	Serial.print("Valve State: ");
	Serial.println(valveState, BIN);
}

void blink()
{
	JLed(LED_PIN).Off().Update();
	delay(100);
	JLed(LED_PIN).On().Update();
}
#endif