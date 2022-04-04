#include <Adafruit_AHTX0.h>
#include "HX711.h"

// Initialize object for interfacing with the DHT20 temperature and humidity sensor
Adafruit_AHTX0 aht;

// Initialize object for interfacing with the HX711 Load Cell Amplifier
HX711 scale;

// Define pins used by the HX711 for data transfer
#define LOADCELL_DOUT_PIN  3
#define LOADCELL_SCK_PIN  2

// Define scale offset obtained from calibration
const float CALIBRATION_FACTOR = -5150;

// Define a number to uniquely identify this arduino (will need to be incremented for each subsequent Arduino that is flashed)
const int UID = 1;

// Constants used in calculating length of delay (in milliseconds)
const unsigned long SECOND_MILLIS = 1000;
const unsigned long HOUR_MILLIS = 3600*SECOND_MILLIS;

void setup() {
  /*
   * Note: the Nano Every has two COM/serial ports, one used via the microUSB port (Serial) and the other used via the RX/TX pins (Serial1).
   *       In this case we are using Serial1 to send data to our Serial-to-TTL USB adapter.
   */
  Serial1.begin(115200);
  Serial1.println("DEBUG: Startup");

  // Configure DHT20 sensor
  if (!aht.begin()) {
    Serial1.println("ERROR: Could not find the DHT20 sensor. Check wiring.");
    while (1) delay(10);
  }
  Serial1.println("DEBUG: DHT20 successfully configured");

  // Configure HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR); // Use pre-determined calibration factor
  scale.tare(); // Set scale to zero
  Serial1.println("DEBUG: HX711 successfully configured");
}

void loop() {  
  // Declare required variables
  sensors_event_t temperature, humidity;
  float weight;

  // Get the humidity and temperature values as sensor events, passing the references to the previously created variables
  aht.getEvent(&humidity, &temperature);

  /*
   * Wait for the HX811 scale to be "ready" before taking readings.
   * If it fails, retry 10 times waiting 100ms between each try.
   */
  if (scale.wait_ready_retry(10, 100)) {
    // Take the average of 50 readings from the load cells
    weight = scale.get_units(50);
  } else {
    weight = -999;
    Serial1.println("WARNING: Failed to get a reading from the HX711");
  }

  /*
   * Send measurement data over serial
   * Format: <ID T H W C>, where ID = an ID that uniquely identifies this Arduino, T = temperature, H = relative humidity, W = weight, C = checksum.
   * Note: temperature is in deg C and humidity is a relative percentage
   */
  Serial1.print("<"); Serial1.print(UID); Serial1.print(" ");
  Serial1.print(temperature.temperature, 2); Serial1.print(" ");
  Serial1.print(humidity.relative_humidity, 2); Serial1.print(" "); // TODO: implement checksum/CRC functionality
  Serial1.print(weight);
  Serial1.println(">");

  // Put HX711 in low-power state until next cycle to conserve power
  scale.power_down();

  // Delay until the next measurement cycle (take measurements four times a day)
  delay(8*HOUR_MILLIS);

  // Wake up the HX711 from its low-power state
  scale.power_up();
}
