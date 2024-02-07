/**
 * @file main.cpp
 * @brief Main program for simulating EKG/ARY signals and managing patient states on an ESP32 device.
 */

#include <Arduino.h>
#include "SPIFFSManager.h"
#include "WiFiWebServer.h"
#include <ESP32TimerInterrupt.h>

// Interval for ISR callback in milliseconds
#define TIMER0_INTERVAL_MS 1

// Pin definitions
#define BPM_STICK_PIN GPIO_NUM_34 ///< BPM input pin.
#define AMP_STICK_PIN GPIO_NUM_35 ///< Amplitude input pin.
#define ARY_SWITCH_PIN GPIO_NUM_26 ///< Button pin to toggle between EKG and ARY modes.
#define KLL_SWITCH_PIN GPIO_NUM_27 ///< Button pin to simulate patient's life status.


// Simulated data arrays
static uint8_t EKG[] = {
    // Simulated EKG data points
	65, 65, 65, 65, 70, 76, 74, 70, 65, 63, 65, 65, 65, 65, 48, 230, 40, 65, 65, 65, 74, 90, 100, 102, 100, 95, 80, 70, 65, 65, 65, 65
};
static uint8_t ARY[] = {
	// Simulated ARY data points
	65, 70, 67, 61, 70, 72, 74, 76, 70, 68, 67, 65, 63, 55, 48, 10, 15, 65, 67, 70, 74, 80, 100, 102, 100, 95, 80, 70, 65, 65, 65, 65
};
static uint8_t deadPoints[] = {
	// Simulated "flatline" data points
	65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65, 65
};

const int maxSize = 512; ///< Maximum size for the FIFO buffer.

/**
 * @class SimpleFIFO
 * @brief Implements a basic FIFO (First In First Out) buffer for data management.
 */
class SimpleFIFO
{
public:
    SimpleFIFO() : frontIndex(0), rearIndex(0), itemCount(0) {}  ///< Constructor initializes FIFO indices and item count.

    bool enqueue(uint8_t data) ///< Adds a data point to the FIFO buffer.
    {
        if (itemCount < maxSize)
        {
            buffer[rearIndex] = data;
            rearIndex = (rearIndex + 1) % maxSize;
            itemCount++;
            return true;
        }
        return false; // Overflow, unable to enqueue
    }

    bool dequeue(uint8_t &data) ///< Removes the oldest data point from the FIFO buffer.
    {
        if (itemCount > 0)
        {
            data = buffer[frontIndex];
            frontIndex = (frontIndex + 1) % maxSize;
            itemCount--;
            return true;
        }
        return false; // Underflow, unable to dequeue
    }

    bool isEmpty() const ///< Checks if the FIFO buffer is empty.
    {
        return itemCount == 0;
    }

private:
    uint8_t buffer[maxSize]; ///< Storage for the FIFO buffer.
    int frontIndex; ///< Index of the oldest data point.
    int rearIndex; ///< Index for the next data point insertion.
    int itemCount; ///< Current number of items in the buffer.
};

// Global state variables
float amp; ///< Amplification factor for signal visualization.
long bpm; ///< Simulated heart rate in beats per minute.
int delayMillis = 9; ///< Delay between data points to control simulation speed.

bool isAlive = true; ///< Indicates if the simulated patient is "alive".
bool isEKG = true; ///< Indicates if the current mode is EKG or ARY.

// Button debouncing variables for ARY_SWITCH_PIN
bool buttonState = false; ///< Current debounced state of the button.
bool lastButtonState = LOW; ///< Previous read state of the button.
unsigned long lastDebounceTime = 0; ///< Last time the button state changed.

// Button debouncing variables for KLL_SWITCH_PIN
bool buttonStateKLL = false; 
bool lastButtonStateKLL = LOW; 
unsigned long lastDebounceTimeKLL = 0; 

unsigned long debounceDelay = 50; ///< Debounce delay in milliseconds.

SimpleFIFO valueFifo; ///< FIFO buffer instance for EKG data management.
ESP32Timer ITimer0(0); ///< Timer instance for periodic ISR callbacks.

/**
 * @brief Timer interrupt service routine for generating EKG data points.
 * 
 * @param timerNo Pointer to timer number (unused).
 * @return true to continue timer operation, false to stop.
 */
bool IRAM_ATTR TimerHandler0(void *timerNo) 
{
    static int xbyte = 0;
    xbyte = (xbyte + 1) % delayMillis;
    if (xbyte != 0)
    {
        return true;
    }

    static int index = 0;
    index = (index + 1) % sizeof(EKG);

    // Based on the simulated patient's status, enqueue the appropriate data points
    if (isAlive)
    {
        if (isEKG)
        {
            valueFifo.enqueue(EKG[index]);
        }
        else
        {
            valueFifo.enqueue(ARY[index]);
        }
    }
    else
    {
        valueFifo.enqueue(deadPoints[index]);
    }

    return true;
}

/**
 * @brief Debounces button presses and toggles a boolean variable.
 * 
 * @param pin Pin number for the button.
 * @param buttonState Reference to the current debounced state of the button.
 * @param lastButtonState Reference to the last read state of the button.
 * @param lastDebounceTime Reference to the last time the button state changed.
 * @param toggleVariable Pointer to the boolean variable to toggle.
 */
void debounceAndToggle(int pin, bool &buttonState, bool &lastButtonState, unsigned long &lastDebounceTime, bool *toggleVariable)
{
    int reading = digitalRead(pin); // Read the current state of the button

    // If the button state has changed, reset the debouncing timer
    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    // If the current time is more than debounceDelay milliseconds since the button state last changed...
    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        // and if the button state has changed...
        if (reading != buttonState)
        {
            buttonState = reading;

            // Only toggle the boolean value if the new button state is LOW (pressed)
            if (buttonState == LOW)
            {
                *toggleVariable = !(*toggleVariable); // Toggle the boolean variable
            }
        }
    }

    lastButtonState = reading; // Save the reading for next time
}

/**
 * @brief Setup function to initialize the device.
 */
void setup()
{
	Serial.begin(115200);

	// Initialize SPIFFS
	initSPIFFS();

	// Read network settings from SPIFFS
	String ssid = readFile(SPIFFS, "/ssid.txt");
	String password = readFile(SPIFFS, "/pass.txt");
	String ip = readFile(SPIFFS, "/ip.txt");
	String gateway = readFile(SPIFFS, "/gateway.txt");

	// Initialize WiFi connection
	initWiFi(ssid, password, ip, gateway);

	pinMode(AMP_STICK_PIN, INPUT);
	pinMode(BPM_STICK_PIN, INPUT);

	pinMode(ARY_SWITCH_PIN, INPUT_PULLUP);
	pinMode(KLL_SWITCH_PIN, INPUT_PULLUP);

	// Start the web server
	startServer();

	ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0);
}

/**
 * @brief Main loop function, called repeatedly.
 */
void loop()
{

	amp = map(analogRead(AMP_STICK_PIN), 0, 4095, 10, 100) / 100.0;
	bpm = map(analogRead(BPM_STICK_PIN), 0, 4095, 40, 220);
	delayMillis = map(analogRead(BPM_STICK_PIN), 0, 4095, 9, 48);

	if (!valueFifo.isEmpty())
	{
		uint8_t val;
		if (valueFifo.dequeue(val))
		{
			notifyClients(val * amp);
		}
	}

	debounceAndToggle(ARY_SWITCH_PIN, buttonState, lastButtonState, lastDebounceTime, &isEKG);

	debounceAndToggle(KLL_SWITCH_PIN, buttonStateKLL, lastButtonStateKLL, lastDebounceTimeKLL, &isAlive);
}
