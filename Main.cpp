/******************************************************
	DeltaT:

	This file contains the source code for a game in
	which the player must stop a moving light at a
	certain point along a line of lights.

 ******************************************************/

// Included libraries:
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <time.h>

using namespace std;

// ------------- [Global constant declarations begin here] ------------- //

const char* const GPIO_EXPORT =                 // Name of the file for
	"sys/class/gpio/export";                    // activating a GPIO pin
const char* GPIO_UNEXPORT =                     // Name of the file for
	"sys/class/gpio/unexport";                  // deactivating a GPIO pin
const char* GPIO_DIRECTORY =                    // Prefix for the directory for
	"sys/class/gpio/gpio";                      // controlling a GPIO pin

const char STAT_FILE[] =                        // Name of the statistics file
	"deltaT.stat";
const char LOG_FILE[] =                         // Name of the log file
	"deltaT.log";

const float TIME_PER_LEVEL = 60;                // Time per level in seconds
const float INITIAL_TIME_PER_LIGHT = 0.4;       // Time per light in seconds
const float SCALING_TIME_PER_LIGHT = 0.50;      // Multiplier for the duration
                                                // a light is on
const float DEFAULT_PAUSE_TIME = 0.5;           // Time the game will pause for
                                                // at the end of a level
const float MAX_IDLE_TIME = 15;                 // Time for which the game will
                                                // idle before exiting
const int TOTAL_NUM_LIGHTS = 9;                 // The total number of lights
                                                // in the strip
const int TARGET_INDEX = 4;                     // Index of the target light
const int INITIAL_NUM_LIVES = 3;                // Initial number of lives
const int MAX_LINE_LENGTH = 100;                // Length of a line in a file
const int TOTAL_NUM_PINS = 10;                  // Total number of available
                                                // pins on the SoC
const int PIN_IDS[10] = {                       // IDs of the pins that will be
	0, 18, 6, 4, 5, 2, 3, 11, 45, 1             // used
};

// -------------- [Global constant declarations end here] -------------- //



// --------------------- [Timer class begins here] --------------------- //

/*************************************************************************
	This class allows the detecting of when a certain time has passed
 *************************************************************************/

class Timer {
	private:
		// The designated ending time
		clock_t stopTime;

	public:
		// Constructor
		Timer () {

			// Initialize stop time to an invalid value
			stopTime = -1;
		}

		// Deconstructor
		~Timer () {}

		// Set timer for some number of seconds in the future
		bool setStopTime (float seconds);

		// Determine whether the timer has finished
		bool isFinished ();
};

// ---------------------- [Timer class ends here] ---------------------- //



// -------------------- [Logger class begins here] --------------------- //

/*************************************************************************
	This class writes log data to file
 *************************************************************************/

class Logger {
	public:
		std::ofstream sysLog;

		// Constructor
		Logger () {
			// Create log file
			sysLog.open(LOG_FILE);

			// Check if file could be opened
			if (!sysLog.is_open()) {
				cerr << "[Logger] ERROR: Log file could not be created." << endl;
			}
		}
};

// --------------------- [Logger class ends here] ---------------------- //



// ------------------ [GPIO Handler class begins here] ----------------- //

/*************************************************************************
	This class handles GPIO interfacing for a single pin
 *************************************************************************/

class GPIOHandler {
	private:
		char* directoryName;    // Directory for controlling a GPIO pin
		int   pinID;            // Identifier for addressing pin
		char* valueFileName;    // Name of the file for controlling a GPIO pin

		ifstream inFile;        // File for generic file reading
		ofstream outFile;       // File for generic file writing

		bool stringFromInt(int num, char*& output);
		int  getLength(const char* string);
		bool concatenate(
			const char* string1, const char* string2, char*& output
		);

	public:
		GPIOHandler(int pinID);
		GPIOHandler();
		~GPIOHandler();
		bool activate();
		bool deactivate();
		bool setType(bool isInput);
		bool getState(bool& state);
		bool setState(bool isOn);
};

// ------------------- [GPIO Handler class ends here] ------------------ //



// Global log object
Logger sysLog;

// Global GPIOHandlers
GPIOHandler* systemPins[TOTAL_NUM_PINS];



// ----------------- [Structure definitions begin here] ---------------- //

// Structure for holding data about the game
struct GameData {
	float timePerLevel;         // This is the duration in seconds for which a
	                            // level lasts
	float timePerLight;         // This is the duration in seconds for which a
								// light should be on
	Timer* levelTimer;          // This is the timer for the length of a level
	Timer* lightTimer;          // This is the timer for the duration a light
	                            // is on
	int currentLevel;           // This counts the number of times the button
								// was pressed at the right time during the
								// current game
	int numLivesRemaining;      // This is the number of attempts remaining
	int currentLightPosition;   // This is the index of the current light that
	                            // is turned on
	bool* lightStates;          // This holds the states of all the lights
	bool isMovingRight;         // Whether or not the light is moving to the
	                            // right
};



// Structure for holding statistics about the game
struct Statistics {
	int highScore;              // This is the highest level reached
	float totalTimePlayed;      // This is the total length of time the game
	                            // has been played
	int timesPressed;           // This is the total number of times the button
	                            // has been pressed
};

// ------------------ [Structure definitions end here] ----------------- //



// ---------------- [Function declarations begin here] ----------------- //

// Functions for hardware interfacing
bool initialize();
void deinitialize();
bool updateLightStrip(bool* lightStates);
int  buttonIsPressed();

// Functions for file input/output
bool readStats(const char* fileName, Statistics* stats);
bool writeStats(const char* fileName, Statistics* stats);

// Functions for changing game data
bool updateLightPosition(GameData* game);
bool updateLightDuration(GameData* game);
bool setRandomDirection (GameData* game);

//Functions for handling game logic
void sleep(float seconds);
bool gameLoopIdle(Statistics* stats);
bool gameLoopPlay(Statistics* stats, GameData* game);

// ----------------- [Function declarations end here] ------------------ //



// ------------- [Functions for the Timer class begin here] ------------ //

// Set timer for some number of seconds in the future
bool Timer::setStopTime (float seconds) {

	// Check for a valid time to add
	if (seconds >= 0) {
		sysLog.sysLog <<
			"[Timer::setStopTime] Setting timer for " <<
			seconds << " second(s) in the future" << endl;

		stopTime = clock() + seconds * CLOCKS_PER_SEC;
	}

	return seconds >= 0;
}

// Determine whether the timer has finished
bool Timer::isFinished () {
	// Check for a valid stop time
	if (stopTime >= 0) {
		// Return whether the current time has passed the
		// designated stop time
		return clock() >= stopTime;

	// Handle invalid stop times
	} else {
		sysLog.sysLog <<
			"[Timer::isFinished] ERROR: Negative stopTime" << endl;

		return false;
	}
}

// -------------- [Functions for the Timer class end here] ------------- //



// --------- [Functions for the GPIOHandler class begin here] ---------- //

// Helper function for GPIOHandler constructor
// Get string representation of int
bool GPIOHandler::stringFromInt (int num, char*& output) {
	sysLog.sysLog << "[GPIOHandler::stringFromInt] " <<
		"Entered function" << endl;

	// Reject negative numbers
	if (num < 0) {
		sysLog.sysLog << "[GPIOHandler::stringFromInt] " <<
			"ERROR: Received negative number" << endl;

		return false;

	// Handle the 0 case
	} else if (num == 0) {
		output = new char[2];
		output[0] = '0';
		output[1] = 0;
	} else {
		int length = floor(log10(num) + 1) + 1;
		output = new char[length];

		// Null-terminate the string
		output[length - 1] = 0;

		// Add each digit to the string
		for (int i = 1; i <= length; i++) {
			output[length - i - 1] = (num % 10) + '0';
			num /= 10;
		}
	}

	sysLog.sysLog << "[GPIOHandler::stringFromInt] " <<
		"Received " << num << "; outputting \"" << output << "\"" << endl;

	return true;
}

// Helper function for GPIOHandler::concatenate
// Get number of characters in a string
int GPIOHandler::getLength (const char* string) {
	sysLog.sysLog << "[GPIOHandler::getLength] " <<
		"Entered function" << endl;

	// Check for null array
	if (string == NULL) {
		sysLog.sysLog << "[GPIOHandler::getLength] " <<
			"ERROR: Received null string" << endl;

		return -1;
	}

	int length = 0;

	// Count number of non-null characters
	while (string[length] != 0) {
		length++;
	}

	sysLog.sysLog << "[GPIOHandler::getLength] " <<
		"Received string of " << length << " characters" << endl;

	return length;
}

// Helper function for GPIOHandler constructor
// Concatenate strings
bool GPIOHandler::concatenate (const char* string1,
		const char* string2, char*& output) {

	sysLog.sysLog << "[GPIOHandler::concatenate] " <<
		"Entered function" << endl;

	int outputStringIndex = 0;
	int inputStringIndex  = 0;

	// Check for null pointers
	if (string1 == NULL || string2 == NULL) {
		sysLog.sysLog << "[GPIOHandler::concatenate] " <<
			"ERROR: Received null string" << endl;

		return false;
	}

	int length = getLength(string1) + getLength(string2) + 1;

	output = new char[length];

	// Copy string1 into outputString
	while (string1[inputStringIndex] != 0) {
		output[outputStringIndex] = string1[inputStringIndex];
		outputStringIndex++;
		inputStringIndex ++;
	}

	inputStringIndex = 0;

	// Copy string2 into outputString
	while (string2[inputStringIndex] != 0) {
		output[outputStringIndex] = string2[inputStringIndex];
		outputStringIndex++;
		inputStringIndex ++;
	}

	// Null-terminate the string
	output[outputStringIndex] = 0;

	sysLog.sysLog << "[GPIOHandler::concatenate] " <<
		"Concatenated string: \"" << output << "\"" << endl;

	return true;
}

// GPIOHandler constructor given pinID
GPIOHandler::GPIOHandler (int pinID) {
	sysLog.sysLog << "[GPIOHandler::GPIOHandler] " <<
		"Entered constructor" << endl;

	this->valueFileName = NULL;
	char* idString;

	// Handle invalid ID
	if (!stringFromInt(pinID, idString) ||
			!concatenate(GPIO_DIRECTORY, idString, this->directoryName)) {

		sysLog.sysLog << "[GPIOHandler::GPIOHandler] " <<
			"ERROR: Received invalid pinID: " << pinID << endl;

		this->pinID = -1;
	} else {

		this->pinID = pinID;
	}
}

// Default GPIOHandler constructor
GPIOHandler::GPIOHandler () {
	sysLog.sysLog << "[GPIOHandler::GPIOHandler] " <<
		"Entered constructor" << endl;

	this->pinID = -1;
	this->directoryName = NULL;
	this->valueFileName = NULL;
}

// GPIOHandler deconstructor
GPIOHandler::~GPIOHandler () {
	delete directoryName;
	directoryName = NULL;
	delete valueFileName;
	valueFileName = NULL;
	pinID = -1;

	if (inFile.is_open()) {
		inFile.close();
	}

	if (outFile.is_open()) {
		outFile.close();
	}
}

// Activate GPIO pin
bool GPIOHandler::activate () {
	sysLog.sysLog << "[GPIOHandler::activate] " <<
		"Entered function" << endl;

	// Check if object is valid
	if (pinID < 0) {
		sysLog.sysLog << "[GPIOHandler::activate] " <<
			"ERROR: Received invalid pinID: " << pinID << endl;

		return false;
	}

	// Check if pin has already been activated
	inFile.open(directoryName);

	if (inFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::activate] " <<
			"WARNING: GPIO pin has already been activated" << endl;

		return true;
	}

	inFile.close();
	outFile.open(GPIO_EXPORT);

	// Check if export file was successfully opened
	if (!outFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::activate] " <<
			"ERROR: Could not open \"" << GPIO_EXPORT << "\"" << endl;

		return false;
	}

	// Activate GPIO pin
	char* stringID;

	sysLog.sysLog << "[GPIOHandler::activate] " <<
		"Getting pinID as a string" << endl;

	// Check for valid pinID
	if (!stringFromInt(pinID, stringID)) {
		sysLog.sysLog << "[GPIOHandler::activate] " <<
			"ERROR: Received invalid pinID " << endl;

		return false;
	}

	outFile << stringID;
	outFile.close();

	return true;
}

// Deactivate GPIO pin
bool GPIOHandler::deactivate () {
	sysLog.sysLog << "[GPIOHandler::deactivate] " <<
		"Entered function" << endl;

	// Check if object is valid
	if (pinID < 0) {
		sysLog.sysLog << "[GPIOHandler::deactivate] " <<
			"ERROR: Received invalid pinID: " << pinID << endl;

		return false;
	}

	// Check if pin has already been deactivated
	inFile.open(directoryName);

	if (!inFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::deactivate] " <<
			"WARNING: GPIO pin has already been deactivated" << endl;

		return true;
	}

	inFile.close();

	if (outFile.is_open()) {
		outFile.close();
	}

	outFile.open(GPIO_UNEXPORT);

	// Check if export file was successfully opened
	if (!outFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::deactivate] " <<
			"ERROR: Could not open " << GPIO_UNEXPORT << endl;

		return false;
	}

	// Deactivate GPIO pin
	char* stringID;

	sysLog.sysLog << "[GPIOHandler::deactivate] " <<
		"Getting pinID as a string" << endl;

	stringFromInt(pinID, stringID);
	outFile << stringID;
	outFile.close();

	return true;
}

// Designate GPIO pin to be either input or output
bool GPIOHandler::setType (bool isInput) {
	const char* IO_DIRECTION_FILE =
		"/direction";

	sysLog.sysLog << "[GPIOHandler::setType] " <<
		"Entered function" << endl;

	// Check if object is valid
	if (pinID < 0) {
		sysLog.sysLog << "[GPIOHandler::setType] " <<
			"ERROR: Received invalid pinID: " << pinID << endl;

		return false;
	}

	char* directionFileName;

	sysLog.sysLog << "[GPIOHandler::setType] " <<
		"Building path name" << endl;

	// Check if path name was built correctly
	if (!concatenate(directoryName, IO_DIRECTION_FILE, directionFileName)) {
		sysLog.sysLog << "[GPIOHandler::setType] " <<
			"ERROR: Path name could not be built" << endl;

		return false;
	}

	outFile.open(directionFileName);

	// Check if file was opened properly
	if (!outFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::setType] " <<
			"ERROR: IO direction file could not be opened" << endl;

		return false;
	}

	if (isInput) {
		outFile << "in";
	} else {
		outFile << "out";
	}

	sysLog.sysLog << "[GPIOHandler::setType] " <<
		"Pin " << pinID << " set to " <<
		((isInput) ? ("input") : ("output")) << endl;

	outFile.close();

	return true;
}

// Get state of pin
bool GPIOHandler::getState (bool& isOn) {
	const char* IO_VALUE_FILE =
		"/value";

	/*sysLog.sysLog << "[GPIOHandler::getState] " <<
		"Entered function" << endl;*/

	// Check if object is valid
	if (pinID < 0) {
		sysLog.sysLog << "[GPIOHandler::getState] " <<
			"ERROR: Received invalid pinID: " << pinID << endl;

		return false;
	}

	// Build path name
	if (valueFileName == NULL) {
		sysLog.sysLog << "[GPIOHandler::getState] " <<
			"Building path name" << endl;

		if (!concatenate(directoryName, IO_VALUE_FILE, valueFileName)) {
			sysLog.sysLog << "[GPIOHandler::getState] " <<
				"ERROR: path name could not be built" << endl;

			return false;
		}
	}

	this->inFile.close();
	this->inFile.open(valueFileName);

	// Check if file could be opened
	if (!this->inFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::getState] " <<
			"ERROR: File could not be opened" << endl;

		return false;
	}

	// Read value from file
	char pinState = 0;

	// Check if value can be read
	if (!this->inFile.get(pinState)) {
		sysLog.sysLog << "[GPIOHandler::getState] " <<
			"ERROR: Value could not be read" << endl;

		return false;
	}

	/*sysLog.sysLog << "[GPIOHandler::getState] " <<
		"Read value \"" << pinState << "\"" << endl;*/

	isOn = (pinState == '1');

	return true;
}

// Set state of pin
bool GPIOHandler::setState (bool isOn) {
	const char* IO_VALUE_FILE =
		"/value";

	// Check if object is valid
	if (pinID < 0) {
		sysLog.sysLog << "[GPIOHandler::setState] " <<
			"ERROR: Received invalid pinID: " << pinID << endl;

		return false;
	}

	/*sysLog.sysLog << "[GPIOHandler::setState][Pin " << pinID << "] " <<
	"Entered function" << endl;*/

	// Build path name
	if (valueFileName == NULL) {
		sysLog.sysLog << "[GPIOHandler::setState][Pin " << pinID << "] " <<
			"Building path name" << endl;

		if (!concatenate(directoryName, IO_VALUE_FILE, valueFileName)) {
			sysLog.sysLog << "[GPIOHandler::setState][Pin " << pinID << "] " <<
				"ERROR: path name could not be built" << endl;

			return false;
		}
	}

	// Open output file
	outFile.close();
	outFile.open(valueFileName);

	// Check if file could be opened
	if (!outFile.is_open()) {
		sysLog.sysLog << "[GPIOHandler::setState][Pin " << pinID << "] " <<
			"ERROR: File could not be opened" << endl;

		return false;
	}

	// Set value
	sysLog.sysLog << "[GPIOHandler::setState][Pin " << pinID << "] " <<
		"Value set to " << (isOn + 0) << endl;

	outFile << ((isOn) ? ("1") : ("0"));

	return true;
}

// ---------- [Functions for the GPIOHandler class end here] ----------- //



// -------- [Functions for interfacing with hardware begin here] ------- //

// Set up the GPIO pins
bool initialize() {
	sysLog.sysLog << "[initialize] " <<
		"Entered function" << endl;

	// Set up GPIO pins
	sysLog.sysLog << "[initialize] " <<
		"Setting up GPIO pins" << endl;

	for (int i = 0; i < TOTAL_NUM_PINS; i++) {
		delete systemPins[i];
		systemPins[i] = new GPIOHandler(PIN_IDS[i]);

		if (!systemPins[i]->activate()) {
			sysLog.sysLog << "[initialize] " <<
				"Failed to activate pin " << PIN_IDS[i] << endl;

			return false;
		}

		// Set first nine pins as output for LEDs
		if (i < TOTAL_NUM_PINS - 1) {
			sysLog.sysLog << "[initialize] " <<
				"Setting pin " << PIN_IDS[i] << " to output" << endl;

			if (!systemPins[i]->setType(false)) {
				sysLog.sysLog << "[initialize] " <<
					"ERROR: Could not set pin " << PIN_IDS[i] <<
					" to output" << endl;

				return false;
			}

			// Set state of pin to false
			sysLog.sysLog << "[initialize] " <<
				"Setting state of pin " << PIN_IDS[i] << " to false" << endl;

			if (!systemPins[i]->setState(false)) {
				sysLog.sysLog << "[initialize] " <<
					"ERROR: Could not set pin " << PIN_IDS <<
					" to false";

				return false;
			}

		// Set last pin as input from button
		} else {
			sysLog.sysLog << "[initialize] " <<
				"Setting pin " << PIN_IDS[i] << " to input" << endl;

			if (!systemPins[i]->setType(true)) {
				sysLog.sysLog << "[initialize] " <<
					"ERROR: Could not set pin " << PIN_IDS[i] << " to input" << endl;

				return false;
			}
		}
	}

	return true;
}

// Ask hardware if the button is pressed
int buttonIsPressed() {
	const int BUTTON_GPIO_PIN_ID = TOTAL_NUM_PINS - 1;
	bool isOn = false;

	/*sysLog.sysLog << "[buttonIsPressed] " <<
		"Entered function" << endl;*/

	// Error check
	if (!systemPins[BUTTON_GPIO_PIN_ID]->getState(isOn)) {
		sysLog.sysLog << "[buttonIsPressed] " <<
			"ERROR: Could not get button state" << endl;

		return -1;
	}

	// Check for button press
	if (isOn) {
		sysLog.sysLog << "[buttonIsPressed] " <<
			"Button is pressed" << endl;

		return 1;
	}

	return 0;
}

// Update which lights are on/off
bool updateLightStrip(bool* lightStates) {
	sysLog.sysLog << "[updateLightStrip] " <<
		"Entered function" << endl;

	// Iterate through lights
	for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
		// Check for errors in changing lights
		if (!systemPins[i]->setState(lightStates[i])) {
			sysLog.sysLog << "[updateLightStrip] " <<
				"ERROR: State of light at pin " << PIN_IDS[i] <<
				" could not be set" << endl;

			return false;
		}
	}

	return true;
}

// Clean up the GPIO pins
void deinitialize() {
	sysLog.sysLog << "[deinitialize] " <<
		"Entered function" << endl;

	// Clean up GPIO pins
	sysLog.sysLog << "[deinitialize] " <<
		"Cleaning up GPIO pins" << endl;

	for (int i = 0; i < TOTAL_NUM_PINS; i++) {
		// Turn off lights
		if (i < TOTAL_NUM_LIGHTS) {
			if (!systemPins[i]->setState(false)) {
				sysLog.sysLog << "[deinitialize] " <<
					"WARNING: Failed to turn off light" <<
					PIN_IDS[i] << endl;
			}
		}

		// Deactivate pin
		if (!systemPins[i]->deactivate()) {
			sysLog.sysLog << "[deinitialize] " <<
				"WARNING: Failed to activate deactivate pin " <<
				PIN_IDS[i] << endl;
		}

		delete systemPins[i];
		systemPins[i] = NULL;
	}
}

// --------- [Functions for interfacing with hardware end here] -------- //



// ----------- [Functions for file input/output begin here] ------------ //

// Read statistics from file
bool readStats(const char* fileName, Statistics* stats) {

	// Check for null pointers
	if (fileName == NULL || stats == NULL) {
		sysLog.sysLog <<
			"[readStats] ERROR: Null pointer found" << endl;
		return false;
	}

	std::ifstream inFile;
	inFile.open(fileName);

	// Check if file could be opened
	if (!inFile.is_open()) {
		sysLog.sysLog <<
			"[readStats] ERROR: Input file could not be opened" << endl;
		return false;
	}

	// Read data from file
	bool done = false;
	int fileLineNumber = 0;
	char* line;

	if (!inFile.getline(line, MAX_LINE_LENGTH)) {
		// Handle end of file
		if (inFile.eof()) {
			sysLog.sysLog <<
				"[readStats] Reached end of file" << endl;
			done = true;

		// Handle error in file
		} else {
			sysLog.sysLog <<
				"[readStats] ERROR: Error in file" << endl;
			return -1;
		}
	}

	return true;
}

bool writeStats(const char* fileName, Statistics* stats) {}

// ------------ [Functions for file input/output end here] ------------- //



// ----------- [Functions for changing game data begin here] ----------- //

// Get a direction "randomly" based on the time the function is called
bool setRandomDirection(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		sysLog.sysLog <<
			"[setRandomDirection] ERROR: Null pointer found" << endl;

		return false;
	}

	bool isMovingRight = (clock() % 2 == 0);

	// Set direction
	game->isMovingRight = isMovingRight;
	sysLog.sysLog <<
		"[setRandomDirection] Direction set to " <<
		((isMovingRight) ? ("right") : ("left")) << endl;

	// Set position according to direction
	if (isMovingRight) {
		game->currentLightPosition = 0;
	} else {
		game->currentLightPosition = TOTAL_NUM_LIGHTS - 1;
	}

	sysLog.sysLog <<
		"[setRandomDirection] Position set to " <<
		(game->currentLightPosition) << endl;

	return true;
}

// Move the light to its next position (cyclic)
bool updateLightPosition(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		sysLog.sysLog <<
			"[updateLightPosition] ERROR: Null pointer found" << endl;
		return false;
	}

	// Move current light to the right
	if (game->isMovingRight) {
		game->currentLightPosition += 1;
		game->currentLightPosition %= TOTAL_NUM_LIGHTS;

		sysLog.sysLog <<
			"[updateLightPosition] Light moved to the right" << endl;

	// Move current light to the left
	} else {
		game->currentLightPosition += TOTAL_NUM_LIGHTS - 1;
		game->currentLightPosition %= TOTAL_NUM_LIGHTS;

		sysLog.sysLog <<
			"[updateLightPosition] Light moved to the left" << endl;
	}

	// Update lightStates array
	for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
		game->lightStates[i] = (i == game->currentLightPosition);
	}

	// Reset light timer
	game->lightTimer->setStopTime(game->timePerLight);

	return true;
}

// Update the length of time for which a light is on
bool updateLightDuration(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		sysLog.sysLog <<
			"[updateLightDuration] ERROR: Null pointer found" << endl;

		return false;
	}

	game->timePerLight *= SCALING_TIME_PER_LIGHT;

	sysLog.sysLog <<
		"[updateLightDuration] Light duration set to " <<
		game->timePerLight << endl;

	return true;
}

// Reset the game
bool reset(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		sysLog.sysLog <<
			"[reset] ERROR: Null pointer found" << endl;

		return false;
	}

	game->timePerLevel = TIME_PER_LEVEL;
	game->timePerLight = INITIAL_TIME_PER_LIGHT;
	game->levelTimer   = new Timer;
	game->lightTimer   = new Timer;
	game->currentLevel = 0;
	game->numLivesRemaining = INITIAL_NUM_LIVES;

	// Initialize array if it does not already exist
	if (game->lightStates == NULL) {
		game->lightStates = new bool[TOTAL_NUM_LIGHTS];

		sysLog.sysLog <<
			"[reset] Initialized lightStates array" << endl;
	}

	// Clear array
	for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
		game->lightStates[i] = false;

		// Turn off lights
		if (!systemPins[i]->setState(false)) {
			return false;
		}
	}

	sysLog.sysLog <<
		"[reset] Cleared lightStates array" << endl;

	return true;
}

// Flash lights
bool flashLights () {
	bool * lightStates = new bool[TOTAL_NUM_LIGHTS];

	// Wait DEFAULT_PAUSE_TIME seconds
	sysLog.sysLog << "[flashLights] " <<
	"Flashing lights" << endl;

	// Set all lights to on
	for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
		lightStates[i] = true;
	}

	if (!updateLightStrip(lightStates)) {
		sysLog.sysLog << "[flashLights] " <<
			"ERROR: Could not turn on light(s)" << endl;

		return false;
	}

	Timer* t = new Timer;

	t->setStopTime(DEFAULT_PAUSE_TIME);

	while (!t->isFinished());

	// Set all lights to off
	for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
		lightStates[i] = false;
	}

	if (!updateLightStrip(lightStates)) {
		sysLog.sysLog << "[flashLights] " <<
			"ERROR: Could not turn off light(s)" << endl;

		return false;
	}

	return true;
}

// ------------ [Functions for changing game data end here] ------------ //



// ----------- [Functions for handling game logic begin here] ---------- //

// Do nothing for some number of seconds
void sleep (float seconds) {
	Timer* t = new Timer;
	t->setStopTime(seconds);

	sysLog.sysLog <<
		"[sleep] Sleeping for " << seconds << " second(s)" << endl;

	while (!t->isFinished());

	sysLog.sysLog <<
		"[sleep] Woke up after " << seconds << " second(s)" << endl;

	delete t;
}

// Do nothing until the button is pressed
bool gameLoopIdle(Statistics* stats, GameData* game) {
	sysLog.sysLog <<
		"[gameLoopIdle] Entered gameLoopIdle state" << endl;

	// Check for null pointer
	if (stats == NULL) {
		sysLog.sysLog <<
			"[gameLoopIdle] ERROR: Null pointer found" << endl;

		return false;
	}

	Timer* t = new Timer;

	sysLog.sysLog <<
		"[gameLoopIdle] Waiting for button press" << endl;

	t->setStopTime(MAX_IDLE_TIME);

	// Get button press
	int buttonPress = buttonIsPressed();

	// Validate button press
	if (buttonPress == -1) {
		sysLog.sysLog << "[gameLoopIdle] " <<
			"ERROR: Button state could not be detected" << endl;

		return false;
	}

	while (buttonPress != 1 && !t->isFinished()) {
		// Get button press
		buttonPress = buttonIsPressed();

		// Check if button press was not detected
		if (buttonPress == -1) {
			sysLog.sysLog << "[gameLoopIdle] " <<
				"ERROR: Button state could not be detected" << endl;

			return false;
		}
	};

	if (!t->isFinished()) {
		delete t;

		sysLog.sysLog << "[gameLoopIdle] " <<
			"Button press detected - exiting idle state" << endl;

		return true;
	} else {
		delete t;

		sysLog.sysLog <<
			"[gameLoopIdle] The button was not pressed for " <<
			MAX_IDLE_TIME << " second(s) - exiting game" << endl;

		return false;
	}
}

// Play the game
bool gameLoopPlay(Statistics* stats, GameData* game) {
	sysLog.sysLog <<
		"[gameLoopPlay] Entered gameLoopPlay state" << endl;

	// Check for null pointers
	if (stats == NULL || game == NULL) {
		sysLog.sysLog <<
			"[gameLoopPlay] ERROR: Null pointer detected" << endl;

		return false;
	}

	sysLog.sysLog <<
		"[gameLoopPlay] Entering life loop" << endl;

	// Loop until there are no lives remaining
	while (game->numLivesRemaining > 0) {
		bool passedLevel = true;

		sysLog.sysLog <<
			"[gameLoopPlay] Entering passedLevel loop" << endl;

		// Loop through levels until the level is failed
		while (passedLevel) {
			bool levelEnded = false;
			passedLevel = false;

			sysLog.sysLog << "[gameLoopPlay] " <<
				"Reset game" << endl;

			reset(game);

			sysLog.sysLog <<
				"[gameLoopPlay] Set random direction" << endl;
			setRandomDirection(game);

			// Handle errors in updating light strip
			if (!updateLightStrip(game->lightStates)) {
				sysLog.sysLog << "[gameLoopPlay] " <<
					"ERROR: Light could not be set" << endl;

				return false;
			}

			// Set initial timer values
			sysLog.sysLog <<
				"[gameLoopPlay] Setting timer stop values" << endl;

			game->lightTimer->setStopTime(game->timePerLight);
			game->levelTimer->setStopTime(game->timePerLevel);

			// Loop through lights until the level is finished
			sysLog.sysLog <<
				"[gameLoopPlay] Entering light-update loop" << endl;

			while (!levelEnded && !game->levelTimer->isFinished()) {

				// Update lights if it is time to update the lights
				if (game->lightTimer->isFinished()) {
					sysLog.sysLog << game->currentLightPosition << endl;

					updateLightPosition(game);

					// Handle errors in updating light strip
					if (!updateLightStrip(game->lightStates)) {
						sysLog.sysLog << "[gameLoopPlay] " <<
						"ERROR: Light could not be set" << endl;

						return false;
					}

					sysLog.sysLog <<
						"[gameLoopPlay] Updating light position" << endl;

					game->lightTimer->setStopTime(game->timePerLight);
				}

				// Check for button press
				int buttonPress = buttonIsPressed();

				// Validate button press
				if (buttonPress == -1) {
					sysLog.sysLog << "[gameLoopPlay] " <<
					"ERROR: Button state could not be detected" << endl;

					return false;

				// Handle button press
				} else if (buttonPress == 1) {
					sysLog.sysLog <<
						"[gameLoopPlay] Button press detected" << endl;

					// Signify that the game has failed if the
					// incorrect light is on
					if ((game->isMovingRight && game->currentLightPosition != TARGET_INDEX + 1) ||
							(!game->isMovingRight && game->currentLightPosition != TARGET_INDEX - 1)) {

						sysLog.sysLog <<
							"[gameLoopPlay] Incorrect position detected: " <<
							game->currentLightPosition << ", expecting " <<
							TARGET_INDEX << endl;

						passedLevel = false;
					} else {
						passedLevel = true;
					}

					levelEnded = true;
				}

				// Level has failed if time runs out
				if (game->levelTimer->isFinished()) {
					levelEnded = true;
					passedLevel = false;
				}
			}

			sysLog.sysLog <<
				"[gameLoopPlay] Exiting life-update loop" << endl;

			// Pause for a moment
			sysLog.sysLog <<
				"[gameLoopPlay] Sleeping for " <<
				DEFAULT_PAUSE_TIME << " second(s)" << endl;

			sleep(DEFAULT_PAUSE_TIME);

			//Check if passedLevel
			if (passedLevel) {
				// Flash lights
				sysLog.sysLog << "[gameLoopPlay] " <<
					"Flash lights to indicate success" << endl;

				flashLights();

				sysLog.sysLog <<
					"[gameLoopPlay] Level passed" << endl;

				// Speed up level
				sysLog.sysLog <<
					"[gameLoopPlay] Speed up level" << endl;
				updateLightDuration(game);

				// Update current level
				game->currentLevel++;
				sysLog.sysLog <<
					"[gameLoopPlay] Current level set to "
					<< game->currentLevel << endl;

				// Update high score
				if (game->currentLevel > stats->highScore) {
					stats->highScore = game->currentLevel;

					sysLog.sysLog <<
						"[gameLoopPlay] Updated high score to " <<
						stats->highScore << endl;
				}
			}
		}

		sysLog.sysLog <<
			"[gameLoopPlay] Exiting passedLevel loop" << endl;

		// Decrement number of lives
		game->numLivesRemaining--;
		sysLog.sysLog <<
			"[gameLoopPlay] Number of lives set to " <<
			game->numLivesRemaining << endl;
	}

	sysLog.sysLog <<
		"[gameLoopPlay] Exiting life loop" << endl;
	sysLog.sysLog <<
		"[gameLoopPlay] Game ended with final score "
		<< game->currentLevel << endl;

	return true;
}

// ------------ [Functions for handling game logic end here] ----------- //



// Set up and run the game:
int main (const int argc, const char* const argv[]) {
	sysLog.sysLog << "[main] " <<
		"Program started" << endl;

	if (!initialize()) {
		sysLog.sysLog << "[main] " <<
			"Exiting game" << endl;

		return -1;
	}

	Statistics* stats = new Statistics;
	GameData* game = new GameData;
	game->lightStates = NULL;

	sysLog.sysLog << "[main] " <<
		"Resetting game" << endl;

	reset(game);

	//Loop while the game has not been idle for MAX_IDLE_TIME
	sysLog.sysLog << "[main] " <<
		"Entering gameLoopIdle state" << endl;

	while (gameLoopIdle(stats, game)) {
		sleep(DEFAULT_PAUSE_TIME);

		sysLog.sysLog << "[main] " <<
			"Entering gameLoopPlay state" << endl;

		gameLoopPlay(stats, game);
	}

	deinitialize();

	sysLog.sysLog << "[main] " <<
		"Exiting game" << endl;

	delete game;

	return 0;
}
