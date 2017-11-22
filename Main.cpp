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

enum gameState {IDLE, IN_PLAY, GAME_OVER};
const char STAT_FILE[] = "deltaT.stat";           // Name of the statistics file
const char LOG_FILE[] = "deltaT.log";             // Name of the log file
const float TIME_PER_LEVEL = 10;                // Time per level in seconds
const float INITIAL_TIME_PER_LIGHT = 0.5;       // Time per light in seconds
const float SCALING_TIME_PER_LIGHT = 0.99;      // Multiplier for the duration
                                                // a light is on
const float DEFAULT_PAUSE_TIME = 1;             // Time the game will pause for
                                                // at the end of a level
const int TOTAL_NUM_LIGHTS = 9;                 // The total number of lights in
                                                // the strip
const int TARGET_INDEX = TOTAL_NUM_LIGHTS / 2;  // Index of the target light
const int INITIAL_NUM_LIVES = 3;                // Initial number of lives
const int MAX_LINE_LENGTH = 100;                // Length of a line in a file
const int TOTAL_NUM_PINS = 10;                  // Total number of available
                                                // pins on the SoC

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

		Logger () {
			// Create log file
			sysLog.open(LOG_FILE);

			// Check if file could be opened
			if (!sysLog.is_open()) {
				cerr << "[Logger] ERROR: Log file could not be created." << endl;
			}
		}

		close () {
			sysLog.close();
		}
};

// --------------------- [Logger class ends here] ---------------------- //



// Global log object
Logger sysLog;



// ----------------- [Structure definitions begin here] ---------------- //

// Structure for holding data about the game
struct GameData {
	gameState currentState;     // This indicates the current state of the game
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
int  updateDisplay(int newDisplayValue);
int  serializeForDisplay(int newDisplayValue);
void updateLightStrip(bool lightStates);
bool buttonIsPressed();

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



// -------- [Functions for interfacing with hardware begin here] ------- //

int updateDisplay(int newDisplayValue) {
	sysLog.sysLog <<
		"[updateDisplay] Updating display to " <<
		 newDisplayValue << endl;
}

int serializeForDisplay(int newDisplayValue) {
	sysLog.sysLog <<
		"[serializeForDisplay] Serializing " << newDisplayValue << endl;
}

bool buttonIsPressed() {
	sysLog.sysLog <<
		"[buttonIsPressed] Button is pressed" << endl;

	return true;
}

void updateLightStrip(bool lightStates) {
	sysLog.sysLog <<
		"[updateLightStrip] Updated light strip" << endl;
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
	char* line = new char[MAX_LINE_LENGTH];

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

	//Set direction
	game->isMovingRight = isMovingRight;
	sysLog.sysLog <<
		"[setRandomDirection] Direction set to " <<
		((isMovingRight) ? ("right") : ("left")) << endl;

	//Set position according to direction
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

	game->currentState = IDLE;
	game->timePerLevel = TIME_PER_LEVEL;
	game->timePerLight = INITIAL_TIME_PER_LIGHT;
	game->levelTimer = new Timer;
	game->lightTimer = new Timer;
	game->currentLevel = 0;
	game->numLivesRemaining = INITIAL_NUM_LIVES;

	// Initialize array if it does not already exist
	if (game->lightStates == NULL) {
		game->lightStates[TOTAL_NUM_LIGHTS];

		sysLog.sysLog <<
			"[reset] Initialized lightStates array" << endl;

	// Clear array if it already exists
	} else {
		for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
			game->lightStates[i] = false;
		}

		sysLog.sysLog <<
			"[reset] Cleared lightStates array" << endl;
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
bool gameLoopIdle(Statistics* stats) {
	// Check for null pointer
	if (stats == NULL) {
		sysLog.sysLog <<
			"[gameLoopIdle] ERROR: Null pointer found" << endl;

		return false;
	}

	sysLog.sysLog << "[gameLoopIdle] Updating display" << endl;
	updateDisplay(stats->highScore);

	sysLog.sysLog <<
		"[gameLoopIdle] Waiting for button press" << endl;

	while (!buttonIsPressed());

	sysLog.sysLog <<
		"[gameLoopIdle] Button press detected - exiting idle state" << endl;

	return true;
}

// Play the game
bool gameLoopPlay(Statistics* stats, GameData* game) {
	// Check for null pointers
	if (stats == NULL || game == NULL) {
		sysLog.sysLog << "[gameLoopPlay] ERROR: Null pointer detected" << endl;

		return false;
	}

	sysLog.sysLog <<
		"[gameLoopPlay] Resetting game" << endl;

	reset(game);

	sysLog.sysLog <<
		"[gameLoopPlay] Entering life loop" << endl;

	// Loop until there are no lives remaining
	while (game->numLivesRemaining > 0) {
		bool passedLevel = true;

		sysLog.sysLog <<
			"[gameLoopPlay] Entering passedLevel loop" << endl;

		// Loop through levels until the level is failed
		while (passedLevel) {
			// Set the display
			sysLog.sysLog <<
				"[gameLoopPlay] Updating display" << endl;

			updateDisplay(game->currentLevel);

			bool levelEnded = false;

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
					sysLog.sysLog <<
						"[gameLoopPlay] Updating light position" << endl;

					game->lightTimer->setStopTime(game->timePerLight);
					updateLightPosition(game);
				}

				// Check for button press
				sysLog.sysLog <<
					"[gameLoopPlay] Checking for button press" << endl;

				if (buttonIsPressed()) {
					sysLog.sysLog <<
						"[gameLoopPlay] Button press detected" << endl;

					// Signify that the game has failed if the
					// incorrect light is on
					if (game->currentLightPosition != TARGET_INDEX) {
						sysLog.sysLog <<
							"[gameLoopPlay] Incorrect position detected" << endl;

						passedLevel = false;
					}

					levelEnded = true;
				}
			}

			sysLog.sysLog <<
				"[gameLoopPlay] Exiting life-update loop" << endl;

			// Pause for a moment
			sysLog.sysLog <<
				"[gameLoopPlay] Sleeping for " <<
				DEFAULT_PAUSE_TIME << " second(s)" << endl;

			sleep(DEFAULT_PAUSE_TIME);

			// Speed up level
			sysLog.sysLog <<
				"[gameLoopPlay] Speed up level" << endl;
			updateLightDuration(game);

			sysLog.sysLog <<
				"[gameLoopPlay] Set random direction" << endl;
			setRandomDirection(game);

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

		sysLog.sysLog <<
			"[gameLoopPlay] Exiting passedLevel loop" << endl;

		// Decrement number of lives
		game->numLivesRemaining--;
		sysLog.sysLog <<
			"[gameLoopPlay] Number of lives set to " <<
			game->numLivesRemaining << endl;
	}

	sysLog.sysLog << "[gameLoopPlay] Exiting life loop" << endl;

	return true;
}

// ------------ [Functions for handling game logic end here] ----------- //



// Set up and run the game:
int main (const int argc, const char* const argv[]) {
	Statistics* stats = new Statistics;
	GameData* game = new GameData;

	/*if (readStats(STAT_FILE, stats)) {

	}*/

	reset(game);

	gameLoopIdle(stats);
	gameLoopPlay(stats, game);

	delete game;

	return 0;
}
