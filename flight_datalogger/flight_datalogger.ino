#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <MS5611.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin Definitions
const int SD_CS_PIN = 10;
const int ONE_WIRE_BUS = 2;

// Sensor Objects
RTC_DS3231 rtc;
MS5611 ms5611(0x77); // MS5611 I2C address
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature probeSensor(&oneWire);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("--- HAB Data Logger Initializing ---");

  // 1. Initialize SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("ERROR: SD Card initialization failed!");
  } else {
    Serial.println("SUCCESS: SD Card ready.");
  }

  // 2. Initialize RTC
  if (!rtc.begin()) {
    Serial.println("ERROR: Couldn't find RTC!");
  } else {
    Serial.println("SUCCESS: RTC ready.");
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // 3. Initialize MS5611
  if (!ms5611.begin()) {
    Serial.println("ERROR: MS5611 sensor not found!");
  } else {
    ms5611.setOversampling(OSR_ULTRA_HIGH);
    Serial.println("SUCCESS: MS5611 pressure sensor ready.");
  }

  // 4. Initialize Temperature Probe
  probeSensor.begin();
  Serial.println("SUCCESS: DS18B20 probe ready.");

  // Create/Open CSV and write header if file is new
  File logFile = SD.open("hab_data.csv", FILE_WRITE);
  if (logFile) {
    if (logFile.size() == 0) {
      logFile.println("Timestamp,Pressure_hPa,Baro_Temp_C,Probe_Temp_C");
    }
    logFile.close();
  }

  Serial.println("--- Setup Complete. Logging started! ---\n");
}

void loop() {
  // Read Time
  DateTime now = rtc.now();
  
  // Read MS5611 Barometer
  ms5611.read();
  
  // Multiply by 2 to compensate for the megaAVR 32-bit register scale bit-shift
  float rawPressure = ms5611.getPressure();
  float pressure = rawPressure * 2.0; 
  float baroTemp = ms5611.getTemperature(); // °C

  // Read DS18B20 Probe
  probeSensor.requestTemperatures();
  float probeTemp = probeSensor.getTempCByIndex(0);

  // Format Timestamp
  char timeBuffer[20];
  snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d", 
           now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  // Print to Serial Monitor
  Serial.print(timeBuffer);
  Serial.print(" | Press: "); Serial.print(pressure); Serial.print(" hPa");
  Serial.print(" | BaroT: "); Serial.print(baroTemp); Serial.print(" C");
  Serial.print(" | ProbeT: "); Serial.print(probeTemp); Serial.println(" C");

  // Log to SD Card
  File logFile = SD.open("hab_data.csv", FILE_WRITE);
  if (logFile) {
    logFile.print(timeBuffer); logFile.print(",");
    logFile.print(pressure);    logFile.print(",");
    logFile.print(baroTemp);    logFile.print(",");
    logFile.println(probeTemp);
    logFile.close(); 
  } else {
    Serial.println("Error writing to hab_data.csv!");
  }

  delay(2000);
}