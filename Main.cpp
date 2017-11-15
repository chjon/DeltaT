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

using namespace std;

// Structure for holding data about the game
struct GameData {
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
	int currentLightStartTime;  // This is the time the current light turned on
	bool direction;             // true/false -> left/right
};



// ---------------- [Function declarations begin here] ----------------- //

int  readData(const char* fileName);
int  writeData(GameData game);
int  updateDisplay(int newDisplayValue);
int  serializeForDisplay(int newDisplayValue);
int  getCurrentLight(GameData game);
bool getRandomDirection(int rngSeed);
bool startLightCycle(GameData game);
bool stopLightCycle(GameData game);
bool buttonIsPressed();
bool updateLightPosition(GameData game);
bool updateLightCycleTime(GameData game, int newCycleTime);
bool setLives(GameData game, int newNumLives);

// ----------------- [Function declarations end here] ------------------ //



// ----------------- [Function definitions begin here] ----------------- //

int  readData (const char* fileName) {}

int  writeData(GameData game) {}

int  updateDisplay(int newDisplayValue) {}

int  serializeForDisplay(int newDisplayValue) {}

int  getCurrentLight(GameData game) {}

bool getRandomDirection(int rngSeed) {}

bool startLightCycle(GameData game) {}

bool stopLightCycle(GameData game) {}

bool buttonIsPressed() {}

bool updateLightPosition(GameData game) {}

bool updateLightCycleTime(GameData game, int newCycleTime) {}

bool setLives(GameData game, int newNumLives) {}

// ------------------ [Function definitions end here] ------------------ //



// Set up and run the game:
int main (const int argc, const char* const argv[]) {

	return 0;
}
