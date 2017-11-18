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
const char DEFAULT_FILE[] = "game.data";

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



// Structure for holding data about the game
struct GameData {
	gameState currentState;     // This indicates the current state of the game
	int highScore;              // This stores the most times the button was
	                            // pressed at the right time during a game
	int currentLevel;           // This counts the number of times the button
	                            // was pressed at the right time during the
								// current game
	int numLivesRemaining;      // This is the number of attempts remaining
	int currentLightPosition;   // This is the index of the current light that
                                // is turned on
	int cycleTime;              // This is the minimum amount of time that must
	                            // pass before the light can switch
	float timePerLight;         // This is the duration in seconds for which a
	                            // light should be on
	bool direction;             // true/false -> left/right
};



// ---------------- [Function declarations begin here] ----------------- //

// Functions for hardware interfacing
int  updateDisplay(int newDisplayValue);
int  serializeForDisplay(int newDisplayValue);
void updateLightStrip(bool lightStates);
bool buttonIsPressed();

// Functions for file input/output
int readData(const char* fileName);
int writeData(GameData game);

// Functions for changing game data
bool setLives(GameData game, int newNumLives);
bool startLightCycle(GameData game);
bool stopLightCycle(GameData game);
bool updateLightPosition(GameData game);
bool updateLightCycleTime(GameData game, int newCycleTime);
bool getRandomDirection(int rngSeed);

// ----------------- [Function declarations end here] ------------------ //



// -------- [Functions for interfacing with hardware begin here] ------- //

int updateDisplay(int newDisplayValue) {}

int serializeForDisplay(int newDisplayValue) {}

bool buttonIsPressed() {}

void updateLightStrip(bool lightStates) {}

// --------- [Functions for interfacing with hardware end here] -------- //



// ----------- [Functions for file input/output begin here] ------------ //

int readData(const char* fileName) {}

int writeData(GameData game) {}

// ------------ [Functions for file input/output end here] ------------- //



// ----------- [Functions for changing game data begin here] ----------- //

bool getRandomDirection(int rngSeed) {}

bool startLightCycle(GameData game) {}

bool stopLightCycle(GameData game) {}

bool updateLightPosition(GameData game) {}

bool updateLightCycleTime(GameData game, int newCycleTime) {}

bool setLives(GameData game, int newNumLives) {}

// ----------- [Functions for changing game data begin here] ----------- //



// Set up and run the game:
int main (const int argc, const char* const argv[]) {

	GameData* game = new GameData;
	game->currentState = IDLE;

	//Main game loop
	while (game->currentState != GAME_OVER) {
		game->currentState = GAME_OVER;
	}

	delete game;

	return 0;
}
