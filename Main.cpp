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
const char STAT_FILE[] = "game.stat";
const char LOG_FILE[] = "game.log";
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
		bool setStopTime (float seconds) {

			// Check for a valid time to add
			if (seconds >= 0) {
				stopTime = clock() + seconds * CLOCKS_PER_SEC;
			}

			return seconds >= 0;
		}

		// Determine whether the timer has finished
		bool isFinished () {

			// Check for a valid stop time
			if (stopTime >= 0) {
				// Return whether the current time has passed the
				// designated stop time
				return clock() >= stopTime;

			// Handle invalid stop times
			} else {
				return false;
			}
		}
};

// ---------------------- [Timer class ends here] ---------------------- //



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
int readData(const char* fileName, Statistics* stats);
int writeData(const char* fileName, Statistics* stats);

// Functions for changing game data
bool updateLightPosition(GameData* game);
bool updateLightDuration(GameData* game);
bool setRandomDirection (GameData* game);

//Functions for handling game logic
void sleep(float seconds);
bool gameLoopIdle(Statistics* stats);
bool gameLoopPlay(Statistics* stats, GameData* game);

// ----------------- [Function declarations end here] ------------------ //



// -------- [Functions for interfacing with hardware begin here] ------- //

int updateDisplay(int newDisplayValue) {}

int serializeForDisplay(int newDisplayValue) {}

bool buttonIsPressed() {}

void updateLightStrip(bool lightStates) {}

// --------- [Functions for interfacing with hardware end here] -------- //



// ----------- [Functions for file input/output begin here] ------------ //

int readData(const char* fileName, Statistics* stats) {}

int writeData(const char* fileName, Statistics* stats) {}

// ------------ [Functions for file input/output end here] ------------- //



// ----------- [Functions for changing game data begin here] ----------- //

// Get a direction "randomly" based on the time the function is called
bool setRandomDirection(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		return false;
	}

	bool isMovingRight = (clock() % 2 == 0);

	//Set direction
	game->isMovingRight = isMovingRight;

	//Set position according to direction
	if (isMovingRight) {
		game->currentLightPosition = 0;
	} else {
		game->currentLightPosition = TOTAL_NUM_LIGHTS - 1;
	}

	return true;
}

// Move the light to its next position (cyclic)
bool updateLightPosition(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		return false;
	}

	// Move current light to the right
	if (game->isMovingRight) {
		game->currentLightPosition += 1;
		game->currentLightPosition %= TOTAL_NUM_LIGHTS;

	// Move current light to the left
	} else {
		game->currentLightPosition += TOTAL_NUM_LIGHTS - 1;
		game->currentLightPosition %= TOTAL_NUM_LIGHTS;
	}

	// Reset light timer
	game->lightTimer->setStopTime(game->timePerLight);

	return true;
}

// Update the length of time for which a light is on
bool updateLightDuration(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
		return false;
	}

	game->timePerLight *= SCALING_TIME_PER_LIGHT;

	return true;
}

// Reset the game
bool reset(GameData* game) {
	// Check for null pointer
	if (game == NULL) {
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

	// Clear array if it already exists
	} else {
		for (int i = 0; i < TOTAL_NUM_LIGHTS; i++) {
			game->lightStates[i] = false;
		}
	}

	return true;
}

// ------------ [Functions for changing game data end here] ------------ //



// ----------- [Functions for handling game logic begin here] ---------- //

// Do nothing for some number of seconds
void sleep (float seconds) {
	Timer* t = new Timer;
	t->setStopTime(seconds);

	while (!t->isFinished());

	delete t;
}

// Do nothing until the button is pressed
bool gameLoopIdle(Statistics* stats) {
	// Check for null pointer
	if (stats == NULL) {
		return false;
	}

	updateDisplay(stats->highScore);

	while (!buttonIsPressed());

	return true;
}

// Play the game
bool gameLoopPlay(Statistics* stats, GameData* game) {
	// Check for null pointers
	if (stats == NULL || game == NULL) {
		return false;
	}

	reset(game);

	// Loop until there are no lives remaining
	while (game->numLivesRemaining > 0) {
		bool passedLevel = true;

		// Loop through levels until the level is failed
		while (passedLevel) {
			// Set the display
			updateDisplay(game->currentLevel);

			bool levelEnded = false;

			// Set initial timer values
			game->lightTimer->setStopTime(game->timePerLight);
			game->levelTimer->setStopTime(game->timePerLevel);

			// Loop through lights until the level is finished
			while (!levelEnded && !game->levelTimer->isFinished()) {

				// Update lights if it is time to update the lights
				if (game->lightTimer->isFinished()) {
					game->lightTimer->setStopTime(game->timePerLight);
					updateLightPosition(game);
				}

				// Check for button press
				if (buttonIsPressed()) {

					// Signify that the game has failed if the
					// incorrect light is on
					if (game->currentLightPosition != TARGET_INDEX) {
						passedLevel = false;
					}

					levelEnded = true;
				}
			}

			// Pause for a moment
			sleep(DEFAULT_PAUSE_TIME);

			// Speed up level
			updateLightDuration(game);
			setRandomDirection(game);

			// Update current level
			game->currentLevel++;

			// Update high score
			if (game->currentLevel > stats->highScore) {
				stats->highScore = game->currentLevel;
			}
		}

		// Decrement number of lives
		game->numLivesRemaining--;
	}

	return true;
}

// ------------ [Functions for handling game logic end here] ----------- //



// Set up and run the game:
int main (const int argc, const char* const argv[]) {
	Statistics* stats = new Statistics;
	GameData* game = new GameData;

	reset(game);

	gameLoopIdle(stats);
	gameLoopPlay(stats, game);

	delete game;

	return 0;
}
