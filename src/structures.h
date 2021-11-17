#ifndef _STRUCTURES
#define _STRUCTURES

// this file contains all the data structures

/**
 * @brief Struct containing all the data relative to a phase
 * 
 */
typedef struct phase
{
  char *name;               // name of the phase
  int id;                   // id of the phase
  int duration;             // duration of the phase
  int repetitions;          // number of repetitions of the phase
  int completed;            // number of time the phase has been completed
  int is_study;             // is this a phase where I should study?
  clock_t started;          // when the phase started
  style fg_color;           // text color of the phase window
  style bg_color;           // background color of the phase window
  struct phase *next;       // pointer to next phase
  struct phase *next_after; // pointer to phase when the repetitions have ended
} Phase;

/**
 * @brief Struct containing all the pointer to cli windows
 * 
 */
typedef struct windows
{
  Window *w_phase;    // window showing current phase
  Window *w_total;    // window showing total time studied
  Window *w_quote;    // window showing a quote
  Window *w_controls; // window showing keyboard controls
  Window *w_paused;   // window telling if the timer is currently paused
} Windows;

/**
 * @brief Struct containing data relative to tone
 * 
 */
typedef struct tone
{
  int repetitions; // number of tones
  int speed;       // int in range [0-10]
} Tone;

/**
 * @brief Struct containing return values from routines
 * 
 */
typedef struct return_values
{
  int show_routine;     // return value from show routine
  int advance_routine;  // return value from advance routine
  int save_routine;     // return value from save routine
  int keypress_routine; // return value from keypress routine
} ReturnValues;

/**
 * @brief Struct containing all parameters for the routines
 * 
 */
typedef struct parameters
{
  int loop;                        // are the routines looping?
  int study_phases;                // amount of currently studied phases
  int windows_force_reload;        // flag to force redraw of the windows
  int phase_elapsed;               // total time elapsed in the current phase
  int study_elapsed;               // total time studied in the session
  int previous_elapsed;            // elapsed loaded from file
  int time_paused, frozen_elapsed; // flag to pause time
  ReturnValues *return_values;     // return values of threads
  Phase *current_phase;            // current phase of the timer
  pthread_mutex_t *terminal_lock;  // terminal lock using mutex
  Windows *windows;                // displayed windows
  Tone *tone;                      // handles tone parameters
} Parameters;

#endif