#ifndef _CONSTANTS
#define _CONSTANTS

// this file contains all the needed constants

static const int STUDYDURATION = 45;      // duration of the study phase, minutes
static const int SHORTBREAKDURATION = 15; // duration of the short break, minutes
static const int LONGBREAKDURATION = 30;  // duration of the long break, minutes
static const int STUDYSESSIONS = 4;       // number of study sessions before long break
static const int Y_BORDER = 1;            // Y border window positioning
static const int PADDING = 2;             // window text padding
static const int BUFLEN = 250;            // length of the buffers
static const int SAVEINTERVAL = 1000;     // interval between saves, msec
static const int SLEEP_INTERVAL = 50;     // threads sleep time, msec

static const char *QUOTES_PATH = ".QUOTES";     // file containing the quotes
static const char *SAVE_PATH = ".SAVE";         // file containing the save for the current sessione
static const char *SETTINGS_PATH = ".SETTINGS"; // file containing settings

#endif