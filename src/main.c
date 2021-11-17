#define _DEFAULT_SOURCE // for nanosleep

#include <stdio.h>
#include <sys/time.h> // for gettimeofday
#include <time.h>     // for timespec
#include <signal.h>   // for signal()
#include <unistd.h>   // for nanosleep()
#include <pthread.h>  // for multithread
#include <string.h>   // for strlen()
#include <stdlib.h>   // for malloc() and free() and rand()

#include "terminal.h"
#include "constants.h"

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

typedef struct windows
{
  Window *w_phase;    // window showing current phase
  Window *w_total;    // window showing total time studied
  Window *w_quote;    // window showing a quote
  Window *w_controls; // window showing keyboard controls
  Window *w_paused;   // window telling if the timer is currently paused
} Windows;

typedef struct
{
  int repetitions; // number of tones
  int speed;       // int in range [0-10]
} Tone;

typedef struct return_values
{
  int show_routine;     // return value from show routine
  int advance_routine;  // return value from advance routine
  int save_routine;     // return value from save routine
  int keypress_routine; // return value from keypress routine
} ReturnValues;

typedef struct
{
  int loop;                        // are the routines looping?
  int study_phases;                // amount of currently studied phases
  int windows_force_reload;        // flag to force redraw of the windows
  int phase_elapsed;               // total time elapsed in the current phase
  int study_elapsed;               // total time studied in the session
  int previous_elapsed;            // elapsed loaded from file
  int time_paused, frozen_elapsed; // flag to pause time
  ReturnValues *ret;               // return values of threads
  Phase *current_phase;            // current phase of the timer
  pthread_mutex_t *terminal_lock;  // terminal lock using mutex
  Windows *windows;                // displayed windows
  Tone *tone;                      // handles tone parameters
} Parameters;

volatile int sigint_called;   // flag for sigint
volatile int sigwinch_called; // flag for sigwinch

int file_read_line(char *buffer, int max_bytes, FILE *fp);
int file_count_lines(FILE *fp);

int check_savefile();
int load_savefile(Parameters *p, Phase *phases);
int save_savefile(Parameters *p);

int check_settings();
int load_settings(int *durations);
int save_settings(int *durations);

void SIGWINCH_handler();
void SIGINT_handler();

unsigned long long epoch();
void ms_sleep(int ms);
void s_sleep(int sec);

Phase *init_phases(Phase *phases, int argc, char *argv[]);
Windows *init_windows();
Parameters *init_parameters(Phase *current_phase, Windows *windows);
void delete_parameters(Parameters *p);
Phase *set_initial_phase(Phase *phases);
void reset_current_time(Parameters *p);
void start_time(Parameters *p);
void pause_time(Parameters *p);
void next_phase(Parameters *p);

void format_elapsed_time(char *buffer, int elapsed);
void format_date(char *buffer);
void format_time_delta(char *buffer, int delta_seconds);

int place_random_quote(Window *w);
void set_windows_visibility(Parameters *p, int visibility);

void *beep_async(void *args);
void *show_routine(void *args);
void *advance_routine(void *args);
void *save_routine(void *args);
void *keypress_routine(void *args);
int check_routines(Parameters *p);

int main(int argc, char *argv[]);

/**
 * @brief Reads a line from file, advance the pointer.
 * 
 * @param buffer pointer to line buffer
 * @param max_bytes maximum number of bytes to read
 * @param fp pointer to file
 * @return int number of read bytes, -1 in case of error
 */
int file_read_line(char *buffer, int max_bytes, FILE *fp)
{
  if (fgets(buffer, max_bytes, fp) == NULL)
    return -1;

  int len = strlen(buffer);
  // remove newline from last character
  buffer[len - 1] = '\0';
  return len;
}

/**
 * @brief Counts number of lines in a file.
 * 
 * @param fp pointer to file
 * @return int number of lines in file
 */
int file_count_lines(FILE *fp)
{
  fpos_t old_pos;
  int lines;

  // save old position
  fgetpos(fp, &old_pos);
  // reset to start
  rewind(fp);

  lines = 0;
  while (!feof(fp))
  {
    char ch = fgetc(fp);
    if (ch == '\n')
      lines++;
  }

  // return to old position
  fsetpos(fp, &old_pos);

  return lines;
}

/**
 * @brief Checks if a save from today is already present.
 * 
 * @return int -1 if file is not found, -2 is save is from another day, 1 if save is valid
 */
int check_savefile()
{
  FILE *fp;
  char buffer[BUFLEN];
  char r_buffer[BUFLEN];

  fp = fopen(SAVE_PATH, "r");
  if (fp == NULL)
    return -1;

  // read date
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -1;

  // format current date
  format_date(buffer);

  if (strcmp(buffer, r_buffer) != 0)
    return -2;

  fclose(fp);

  return 1;
}

/**
 * @brief Loads save from file.
 * 
 * @param p pointer to parameters structs
 * @param phases pointer to phases
 * @return int -1 if file is not found, negative values for other errors, 0 if save is correctly loaded
 */
int load_savefile(Parameters *p, Phase *phases)
{
  FILE *fp;
  char r_buffer[BUFLEN];
  int num_buffer;

  // if the save is not from today, return
  if (check_savefile() == -1)
    return -2;

  fp = fopen(SAVE_PATH, "r");
  if (fp == NULL)
    return -1;

  // read date, discarding first line
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -1;

  // read phase elapsed
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -2;

  // update the parameters struct
  p->phase_elapsed = atoi(r_buffer);

  // read total study elapsed
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -3;
  // update the parameters struct
  p->previous_elapsed = atoi(r_buffer);

  // read total study phases
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -4;
  // update the parameters struct
  p->study_phases = atoi(r_buffer);

  // read current phase id
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -5;

  // update the parameters struct
  num_buffer = atoi(r_buffer);
  // cycle to find the correct phase
  for (int i = 0; i < 3; i++)
  {
    if (phases[i].id == num_buffer)
    {
      p->current_phase = phases + i;
      break;
    }

    return -6;
  }

  // read current phase completed
  if (file_read_line(r_buffer, BUFLEN, fp) == -1)
    return -7;
  // update the parameters struct
  p->current_phase->completed = atoi(r_buffer);

  fclose(fp);

  return 0;
}

/**
 * @brief Saves stats to file.
  Structure of the save file:
  - timestamp (year-month-day)
  - phase elapsed
  - total study elapsed
  - total study phases
  - current phase id
  - current phase completed
  - current phase started
 * 
 * @param p pointer to parameters
 * @return int -1 in case of error, 0 in case of success
 */
int save_savefile(Parameters *p)
{
  FILE *fp;
  char w_buffer[BUFLEN];

  fp = fopen(SAVE_PATH, "w");
  if (fp == NULL)
    return -1;

  // save timestamp
  format_date(w_buffer);
  fputs(w_buffer, fp);
  fputc('\n', fp);

  // save phase elapsed
  sprintf(w_buffer, "%i\n", p->phase_elapsed);
  fputs(w_buffer, fp);

  // save total study elapsed
  sprintf(w_buffer, "%i\n", p->study_elapsed);
  fputs(w_buffer, fp);

  // save total study phases
  sprintf(w_buffer, "%i\n", p->study_phases);
  fputs(w_buffer, fp);

  // save current phase id
  sprintf(w_buffer, "%i\n", p->current_phase->id);
  fputs(w_buffer, fp);

  // save current completed phases
  sprintf(w_buffer, "%i\n", p->current_phase->completed);
  fputs(w_buffer, fp);

  // save current phase started
  sprintf(w_buffer, "%li\n", p->current_phase->started);
  fputs(w_buffer, fp);

  fclose(fp);

  return 0;
}

/**
 * @brief Checks if settings file exists.
 * 
 * @return int -1 if file does not exist, -2 if it's not valid, 0 if it's valid
 */
int check_settings()
{
  FILE *fp;

  fp = fopen(SETTINGS_PATH, "r");

  if (fp == NULL)
    return -1;

  // check if there are enough lines
  if (file_count_lines(fp) < 4)
    return -2;

  fclose(fp);

  return 0;
}

/**
 * @brief Loads settings from file.
 * 
 * @param durations pointer to array to hold the lines
 * @return int -1 if file does not exist, -2 if it's not valid, 0 if it's valid
 */
int load_settings(int *durations)
{
  FILE *fp;
  char r_buffer[BUFLEN];

  fp = fopen(SETTINGS_PATH, "r");

  if (fp == NULL)
    return -1;

  // check if there are enough lines
  if (file_count_lines(fp) < 4)
    return -2;

  // populate array with integers loaded from file
  for (int i = 0; i < 4; i++)
  {
    file_read_line(r_buffer, BUFLEN, fp);
    //! ATOI does not check if the value is valid
    *(durations + i) = atoi(r_buffer);
  }

  fclose(fp);

  return 0;
}

/**
 * @brief Save settings to file.
 * 
 * @param durations pointer to duration 
 * @return int -1 if file is not writeable, 0 otherwise
 */
int save_settings(int *durations)
{
  FILE *fp;
  char w_buffer[BUFLEN];

  fp = fopen(SETTINGS_PATH, "w");
  if (fp == NULL)
    return -1;

  // save study duration
  sprintf(w_buffer, "%i\n", *durations);
  fputs(w_buffer, fp);

  // save short break duration
  sprintf(w_buffer, "%i\n", *(durations + 1));
  fputs(w_buffer, fp);

  // save long break duration
  sprintf(w_buffer, "%i\n", *(durations + 2));
  fputs(w_buffer, fp);

  // save study sessions
  sprintf(w_buffer, "%i\n", *(durations + 3));
  fputs(w_buffer, fp);

  fclose(fp);

  return 0;
}

/**
 * @brief Handler for CTRL+C.
 * 
 */
void SIGINT_handler()
{
  sigint_called = 1;
  return;
}

/**
 * @brief Handler for terminal resize.
 * 
 */
void SIGWINCH_handler()
{
  sigwinch_called = 1;
  return;
}

/**
 * @brief Returns time (milliseconds) since epoch
 * 
 * @return unsigned long long 
 */
unsigned long long epoch()
{
  //! this might not be windows compatible
  struct timeval tv;

  gettimeofday(&tv, NULL);

  unsigned long long ms_since_epoch =
      (unsigned long long)(tv.tv_sec) * 1000 +
      (unsigned long long)(tv.tv_usec) / 1000;

  return ms_since_epoch;
}

/**
 * @brief Sleeps a set amount of time (milliseconds).
 * 
 * @param ms millisecond
 */
void ms_sleep(int ms)
{
  // check if number of milli second is valid
  if (ms < 0)
    return;

  // pack a struct
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  // actually sleep
  nanosleep(&ts, NULL);
}

/**
 * @brief Sleeps a set amount of time (seconds).
 * 
 * @param s seconds
 */
void s_sleep(int s)
{
  // check if number of second is valid
  if (s < 0)
    return;

  // sleep...
  ms_sleep(s * 1000);
}

/**
 * @brief Assigns all the variables needed to run the timer.
 * 
 * @param phases pointer to phases array
 * @param argc 
 * @param argv 
 * @return Phase* current phase
 */
Phase *init_phases(Phase *phases, int argc, char *argv[])
{
  // preload durations to default values
  int durations[] = {
      STUDYDURATION,
      SHORTBREAKDURATION,
      LONGBREAKDURATION,
      STUDYSESSIONS,
  };

  if (argc > 1)
  {
    // if arguments have been passed, load them
    if (strcmp(argv[1], "reset") != 0)
    {
      for (int i = 1; i < argc && i < 5; i++)
        durations[i - 1] = atoi(argv[i]);
    }
  }
  else if (check_settings() == 0)
  {
    // otherwise, just load from file
    load_settings(durations);
  }

  save_settings(durations);

  // create array of phases
  phases[0] = (Phase){
      .name = "study",
      .id = 0,
      .duration = durations[0],
      .repetitions = durations[3],
      .completed = 0,
      .started = 0,
      .is_study = 1,
      .next = phases + 1,
      .next_after = phases + 2,
      .fg_color = fg_RED,
      .bg_color = bg_DEFAULT,
  };

  phases[1] = (Phase){
      .name = "short break",
      .id = 1,
      .duration = durations[1],
      .repetitions = 0,
      .completed = 0,
      .started = 0,
      .is_study = 0,
      .next = phases,
      .fg_color = fg_GREEN,
      .bg_color = bg_DEFAULT,
  };

  phases[2] = (Phase){
      .name = "long break",
      .id = 2,
      .duration = durations[2],
      .repetitions = 0,
      .completed = 0,
      .started = 0,
      .is_study = 0,
      .next = phases,
      .fg_color = fg_GREEN,
      .bg_color = bg_DEFAULT,
  };

  phases->started = time(NULL);
  return phases; // returns current phase
}

/**
 * @brief Creates the windows container struct and returns its pointer.
 * 
 * @return Windows* 
 */
Windows *init_windows()
{
  Window *w_phase, *w_total, *w_quote, *w_controls, *w_paused;
  Windows *w;

  // first, allocate space for the windows struct
  w = malloc(sizeof(Windows));

  // now create all the windows

  // w_phase keeping track of current phase
  w_phase = createWindow(0, Y_BORDER);
  windowSetAlignment(w_phase, 0);
  windowSetPadding(w_phase, PADDING);
  windowSetFGcolor(w_phase, fg_RED);

  // w_phase keeping track of total time
  w_total = createWindow(0, 0);
  windowSetAlignment(w_total, 0);
  windowSetPadding(w_total, PADDING);
  windowSetFGcolor(w_total, fg_BRIGHT_YELLOW);

  // w_quote with... a quote
  w_quote = createWindow(0, Y_BORDER);
  windowSetAlignment(w_quote, 0);
  windowSetPadding(w_quote, PADDING);
  windowSetAutoWidth(w_quote, 0);
  windowSetFGcolor(w_quote, fg_BRIGHT_BLUE);
  windowSetTextStyle(w_quote, text_ITALIC);
  place_random_quote(w_quote);

  // window with info
  w_controls = createWindow(0, 0);
  windowSetAlignment(w_controls, 0);
  windowSetPadding(w_controls, PADDING);
  windowSetAutoWidth(w_controls, 0);
  windowSetFGcolor(w_controls, fg_BRIGHT_GREEN);
  windowAddLine(w_controls, "press S to skip, P to pause, Q to get and new quote, I to hide this window, CTRL+C to exit");

  // window showing is timer is currently paused
  w_paused = createWindow(0, 0);
  windowSetAlignment(w_paused, 0);
  windowSetPadding(w_paused, PADDING);
  windowSetAutoWidth(w_paused, 0);
  windowSetAutoHeight(w_paused, 0);
  windowSetFGcolor(w_paused, fg_BRIGHT_RED);
  windowSetTextStyle(w_paused, text_BLINKING);
  windowSetVisibility(w_paused, 0);
  windowAddLine(w_paused, "WARNING, TIMER IS CURRENTLY PAUSED");

  // assign the windows to the struct
  w->w_phase = w_phase;
  w->w_total = w_total;
  w->w_quote = w_quote;
  w->w_controls = w_controls;
  w->w_paused = w_paused;

  // return pointer to window
  return w;
}

/**
 * @brief Inits parameters.
 * 
 * @param current_phase 
 * @return Parameters* 
 */
Parameters *init_parameters(Phase *current_phase, Windows *windows)
{
  // allocate space for all parameters
  Parameters *p = malloc(sizeof(Parameters));
  // create mutex
  p->terminal_lock = malloc(sizeof(*(p->terminal_lock)));
  pthread_mutex_init(p->terminal_lock, NULL);

  // create windows
  p->windows = windows;

  // create tone
  p->tone = malloc(sizeof(*(p->tone))); //? can this be cleaned?
  p->tone->repetitions = 0;
  p->tone->speed = 0;

  // create return values
  p->ret = malloc(sizeof(*(p->ret))); //? can this be cleaned?
  p->ret->show_routine = 1;
  p->ret->advance_routine = 1;
  p->ret->save_routine = 1;
  p->ret->keypress_routine = 1;

  // init all other parameters
  p->current_phase = current_phase;
  p->study_phases = 0;
  p->study_elapsed = 0;
  p->windows_force_reload = 1;
  p->phase_elapsed = 0;
  p->study_elapsed = 0;
  p->previous_elapsed = 0;
  p->time_paused = 1;
  p->frozen_elapsed = 0;

  return p;
}

/**
 * @brief Deletes parameters struct.
 * 
 * @param p pointer to parameters struct
 */
void delete_parameters(Parameters *p)
{
  // free memory and delete mutex
  pthread_mutex_destroy(p->terminal_lock);
  free(p->terminal_lock);

  // free memory and delete windows
  deleteWindow(p->windows->w_phase);
  deleteWindow(p->windows->w_total);
  deleteWindow(p->windows->w_quote);
  deleteWindow(p->windows->w_controls);
  deleteWindow(p->windows->w_paused);
  free(p->windows);

  // free memory from tone
  free(p->tone);

  // free memory from return values
  free(p->ret);

  // free memory from parameters
  free(p);
}

/**
 * @brief Resets current phase time.
 * 
 * @param p pointer to parameters struct
 */
void reset_current_time(Parameters *p)
{
  p->current_phase->started = time(NULL);
}

/**
 * @brief Starts counting time.
 * 
 * @param p pointer to parameters struct
 */
void start_time(Parameters *p)
{
  p->time_paused = 0;
}

/**
 * @brief Pauses current phase.
 * 
 * @param p pointer to parameters struct
 */
void pause_time(Parameters *p)
{
  p->time_paused = 1;
  p->frozen_elapsed = (time(NULL) - p->current_phase->started);
}

/**
 * @brief Advances to next phase.
 * 
 * @param p pointer to parameters struct
 */
void next_phase(Parameters *p)
{
  pthread_t beep_thread;
  pthread_create(&beep_thread, NULL, beep_async, p);
  pthread_detach(beep_thread);

  // one phase has been completed
  p->current_phase->completed++;

  // shall the phase count toward the maximum?
  // yes
  if (p->current_phase->is_study)
  {
    p->study_phases++;
    // update the total time until now
    p->previous_elapsed += p->phase_elapsed;
  }

  // check if this phase must be repeated
  if (p->current_phase->completed >= p->current_phase->repetitions && p->current_phase->repetitions)
  {
    // repetitions have ended
    p->current_phase->completed = 0;
    p->current_phase = p->current_phase->next_after;
  }
  else
  {
    // no repetitions or repetitions already finished
    p->current_phase = p->current_phase->next;
  }

  reset_current_time(p);
}

/**
 * @brief Formats elapsed time.
 * 
 * @param buffer pointer to buffer string
 * @param elapsed elapsed time in seconds
 */
void format_elapsed_time(char *buffer, int elapsed)
{
  int seconds, minutes, hours;
  // unpack time into hours, minutes, seconds
  hours = elapsed / 3600;
  minutes = (elapsed % 3600) / 60;
  seconds = elapsed % 60;

  if (hours > 0)
  {
    // if hours is bigger than 0, save into final string
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
  }
  else
  {
    // otherwise, just neglect it
    sprintf(buffer, "%02d:%02d", minutes, seconds);
  }

  return;
}

/**
 * @brief Formats tile difference (in seconds).
 * 
 * @param pointer to buffer string
 * @param delta_seconds elapsed time in seconds
 */
void format_time_delta(char *buffer, int delta_seconds)
{
  time_t now;
  struct tm tm;
  now = time(NULL) + (time_t)delta_seconds;
  tm = *localtime(&now);
  sprintf(buffer, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  return;
}

/**
 * @brief Formats date (discarding time).
 * 
 * @param pointer to buffer string
 */
void format_date(char *buffer)
{
  time_t now;
  struct tm tm;
  now = time(NULL);
  tm = *localtime(&now);
  sprintf(buffer, "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

/**
 * @brief Gets random quote and saves it into window.
 * 
 * @param w pointer to window
 * @return int -1 if file is not readable, 0 in case of success
 */
int place_random_quote(Window *w)
{
  char r_buffer[BUFLEN], l_buffer[BUFLEN];
  int rindex, count;
  FILE *fp;

  count = 0;
  fp = fopen(QUOTES_PATH, "r");

  if (fp == NULL)
  {
    // if the quotes file is somehow not found,
    // add a gimmick quote to panel
    windowAddLine(w, "God is dead and we killed him.");
    windowAddLine(w, "~Harambe");
    return -1;
  }

  // reset quotes window
  windowDeleteAllLines(w);

  // calculate the number of lines
  const int quotes_num = file_count_lines(fp);
  rindex = random() % quotes_num;

  while (fgets(r_buffer, BUFLEN, fp) != NULL)
  {

    if (count == rindex)
    {
      int line_end;
      // remove newline and find end
      for (int i = 0; i < BUFLEN; i++)
      {
        if (r_buffer[i] == '\n')
        {
          line_end = i;
          break;
        }
      }

      // find author and remove it from string
      int author_start;
      for (int i = line_end; i >= 0; i--)
      {
        if (r_buffer[i] == '@')
        {
          author_start = i + 1;
          break;
        }
      }

      // copy quote to buffer
      for (int i = 0; i < author_start - 1; i++)
        l_buffer[i] = r_buffer[i];
      l_buffer[author_start - 1] = '\0';

      // copy quote into destination
      windowAddLine(w, l_buffer);

      // copy author to buffer
      for (int i = author_start; i < line_end; i++)
      {
        l_buffer[i - author_start] = r_buffer[i];
      }
      l_buffer[line_end - author_start] = '\0';
      // copy author into destination
      windowAddLine(w, l_buffer);

      break;
    }

    count++;
  }

  fclose(fp);

  return 0;
}

/* hides/shows all windows on screen */
/**
 * @brief Sets the windows visibility.
 * 
 * @param p pointer to parameters
 * @param visibility 0 for invisible, 1 for visible
 */
void set_windows_visibility(Parameters *p, int visibility)
{
  clear_terminal();
  windowSetVisibility(p->windows->w_phase, visibility);
  windowSetVisibility(p->windows->w_total, visibility);
  windowSetVisibility(p->windows->w_quote, visibility);
  windowSetVisibility(p->windows->w_paused, visibility);
}

/**
 * @brief Async beeping.
 * 
 * @param args 
 * @return void* 
 */
void *beep_async(void *args)
{
  Parameters *p = args;
  const int delay = (1 - p->tone->speed / 10.0) * 700 + 300;
  for (int i = 0; i < p->tone->repetitions; i++)
  {
    pthread_mutex_lock(p->terminal_lock);
    terminal_beep();
    pthread_mutex_unlock(p->terminal_lock);
    ms_sleep(delay);
  }
  pthread_exit(0);
}

/**
 * @brief Routine handling terminal output.
 * 
 * @param args 
 * @return void* 
 */
void *show_routine(void *args)
{
  Parameters *p = args;
  unsigned long long last_updated = 0;

  while (p->loop)
  {

    // update once a second
    if (epoch() - last_updated > 250)
    {
      last_updated = epoch();

      char buffer[BUFLEN];
      char num_buffer[BUFLEN];

      // remove old lines
      windowDeleteAllLines(p->windows->w_phase);
      windowDeleteAllLines(p->windows->w_total);

      // update windows color
      windowSetFGcolor(p->windows->w_phase, p->current_phase->fg_color);

      // first line of phase window
      if (p->current_phase->repetitions > 0)
        sprintf(buffer, "current phase: %s [%i/%i]", p->current_phase->name, p->current_phase->completed + 1, p->current_phase->repetitions);
      else
        sprintf(buffer, "current phase: %s", p->current_phase->name);
      windowAddLine(p->windows->w_phase, buffer);

      // second line of phase window
      sprintf(buffer, "phase duration: %i minutes", p->current_phase->duration);
      windowAddLine(p->windows->w_phase, buffer);

      // format time
      format_elapsed_time(num_buffer, p->phase_elapsed);
      // third line of phase window
      sprintf(buffer, "elapsed time: %s", num_buffer);
      windowAddLine(p->windows->w_phase, buffer);

      // first line of w_total
      sprintf(buffer, "total study sessions: %i", p->study_phases);
      windowAddLine(p->windows->w_total, buffer);

      // second line of w_total
      format_elapsed_time(num_buffer, p->study_elapsed);
      sprintf(buffer, "total time studied: %s", num_buffer);
      windowAddLine(p->windows->w_total, buffer);

      // third line of w_total
      int time_remaining = p->current_phase->duration * 60 - (time(NULL) - p->current_phase->started);
      format_time_delta(num_buffer, time_remaining);
      sprintf(buffer, "phase ending: %s", num_buffer);
      windowAddLine(p->windows->w_total, buffer);

      // wait for exclusive terminal use
      pthread_mutex_lock(p->terminal_lock);
      windowShow(p->windows->w_phase);
      windowShow(p->windows->w_total);
      pthread_mutex_unlock(p->terminal_lock);
    }

    if (p->windows_force_reload)
    {
      // get largest window
      int largest, terminal_width, dx;
      largest = windowGetSize(p->windows->w_phase).width + windowGetSize(p->windows->w_total).width + 1;
      // now get terminal width
      terminal_width = get_terminal_size().width;
      // now calculate displacement
      dx = (terminal_width - largest) / 2;

      // set position of w_phase
      windowSetPosition(p->windows->w_phase, dx, Y_BORDER);
      // set position of w_total
      windowSetPosition(p->windows->w_total, windowGetBottomRight(p->windows->w_phase).x + 1, Y_BORDER);
      windowAutoResize(p->windows->w_total); // trigger resize to get the actual width

      Position total_br_corner = windowGetBottomRight(p->windows->w_total);
      // set position of w_quote
      windowSetPosition(p->windows->w_quote, dx, total_br_corner.y);
      windowSetSize(p->windows->w_quote, total_br_corner.x - dx, 4);
      windowAutoResize(p->windows->w_quote); // trigger resize to get the actual width

      Position quotes_br_corner = windowGetBottomRight(p->windows->w_quote);
      // set position of w_controls
      windowSetPosition(p->windows->w_controls, dx, quotes_br_corner.y);
      windowSetWidth(p->windows->w_controls, total_br_corner.x - dx);
      windowAutoResize(p->windows->w_controls); // trigger resize to get the actual width

      // now set position of w_paused
      if (windowGetVisibility(p->windows->w_controls))
      {
        // since info window is visible, get its position
        Position info_br_corner = windowGetBottomRight(p->windows->w_controls);
        windowSetPosition(p->windows->w_paused, dx, info_br_corner.y);
      }
      else
      {
        // otherwise, place below quotes
        windowSetPosition(p->windows->w_paused, dx, quotes_br_corner.y);
      }

      windowSetWidth(p->windows->w_paused, quotes_br_corner.x - dx);
      windowSetHeight(p->windows->w_paused, 3);
      windowSetVisibility(p->windows->w_paused, p->time_paused);

      // wait for exclusive use of terminal
      pthread_mutex_lock(p->terminal_lock);
      // clear terminal
      clear_terminal();

      // display all
      windowShow(p->windows->w_phase);
      windowShow(p->windows->w_total);
      windowShow(p->windows->w_quote);
      windowShow(p->windows->w_controls);
      windowShow(p->windows->w_paused);

      // unlock terminal
      pthread_mutex_unlock(p->terminal_lock);

      // reset flag
      p->windows_force_reload = 0;
    }

    // idle
    ms_sleep(SLEEP_INTERVAL);
  }
  p->ret->show_routine = 0;
  pthread_exit(0);
}

/**
 * @brief Routine handling time.
 * 
 * @param args 
 * @return void* 
 */
void *advance_routine(void *args)
{
  Parameters *p = args;
  while (p->loop)
  {
    if (p->time_paused)
    {
      // stall elapsed time
      p->current_phase->started = time(NULL) - p->frozen_elapsed;
    }
    else
    {
      int phase_elapsed;
      // calculate seconds elapsed
      phase_elapsed = time(NULL) - p->current_phase->started;

      // second line of w_total, total time spent studying
      int total_elapsed = 0;
      // add current session to total time if it's a study session
      if (p->current_phase->is_study)
      {
        total_elapsed = phase_elapsed;
        // next phase is a study phase, set tone accordingly
        p->tone->speed = 3;
        p->tone->repetitions = 3;
      }
      else
      {
        // the next phase is a pause, set tone accordingly
        p->tone->speed = 10;
        p->tone->repetitions = 5;
      }

      // update the parameters struct
      // time in the current phase
      p->phase_elapsed = phase_elapsed;
      // total time studied today, until this very moment
      p->study_elapsed = total_elapsed + p->previous_elapsed;

      if (phase_elapsed / 60 >= p->current_phase->duration)
      {
        // phase has been completed
        // go to next
        next_phase(p);
        // load a new quote
        place_random_quote(p->windows->w_quote);
        // force windows reload
        p->windows_force_reload = 1;
      }
    }

    ms_sleep(SLEEP_INTERVAL);
  }
  p->ret->advance_routine = 0;
  pthread_exit(0);
}

/**
 * @brief Routing handling periodic saves.
 * 
 * @param args 
 * @return void* 
 */
void *save_routine(void *args)
{
  Parameters *p = args;
  unsigned long long last_save = 0;
  while (p->loop)
  {

    if (epoch() - last_save > SAVEINTERVAL)
    {
      save_savefile(p);
      last_save = epoch();
    }

    ms_sleep(SLEEP_INTERVAL);
  }

  p->ret->save_routine = 0;
  pthread_exit(0);
}

/**
 * @brief Routine handling keypresses.
 * 
 * @param args 
 * @return void* 
 */
void *keypress_routine(void *args)
{
  Parameters *p = args;

  while (p->loop)
  {
    if (sigint_called)
    {
      sigint_called = 0;

      // hide all windows
      // wait for exclusive use of terminal
      pthread_mutex_lock(p->terminal_lock);
      set_windows_visibility(p, 0);
      // unlock terminal
      pthread_mutex_unlock(p->terminal_lock);

      Dialog *d;
      int ret;

      d = createDialog(0, Y_BORDER);
      dialogSetPadding(d, 4);
      dialogSetText(d, "Exit pomodoro?", 1);
      dialogCenter(d, 1, 0);
      dialogShow(d);
      ret = dialogWaitResponse(d);
      dialogClear(d);
      deleteDialog(d);

      if (ret == 1)
      {
        p->loop = 0;
      }
      else
      {
        // show windows again
        // wait for exclusive use of terminal
        pthread_mutex_lock(p->terminal_lock);
        set_windows_visibility(p, 1);
        // unlock terminal
        pthread_mutex_unlock(p->terminal_lock);

        p->windows_force_reload = 1;
      }
    }
    else if (sigwinch_called)
    {
      // terminal has been resized
      sigwinch_called = 0;
      p->windows_force_reload = 1;
    }

    char key;
    // wait for exclusive use of terminal
    pthread_mutex_lock(p->terminal_lock);
    key = poll_keypress();
    // unlock terminal
    pthread_mutex_unlock(p->terminal_lock);

    // transform to lower case
    if (key >= 65 && key <= 90)
      key += 32;

    if (key == 'p')
    {
      if (!p->time_paused)
        pause_time(p);
      else
        start_time(p);

      // force windows refresh
      p->windows_force_reload = 1;
    }
    else if (key == 's')
    {
      // hide all windows
      // wait for exclusive use of terminal
      pthread_mutex_lock(p->terminal_lock);
      set_windows_visibility(p, 0);
      // unlock terminal
      pthread_mutex_unlock(p->terminal_lock);

      // pause time so phases does not elapse
      // while dialog is shown
      pause_time(p);

      Dialog *d;
      int ret;

      d = createDialog(0, Y_BORDER);
      dialogSetPadding(d, 4);
      dialogSetText(d, "Do you want to skip the current session?", 1);
      dialogCenter(d, 1, 0);
      dialogShow(d);
      ret = dialogWaitResponse(d);
      dialogClear(d);
      deleteDialog(d);

      if (ret)
      { // load stats from file
        next_phase(p);
      }

      // show windows again
      // wait for exclusive use of terminal
      pthread_mutex_lock(p->terminal_lock);
      clear_terminal();
      set_windows_visibility(p, 1);
      // unlock terminal
      pthread_mutex_unlock(p->terminal_lock);

      // restart time
      start_time(p);
      // update flags
      p->windows_force_reload = 1;
      // reset tone
      p->tone->repetitions = 1;
    }
    else if (key == 'q')
    {
      // make a new quote
      place_random_quote(p->windows->w_quote);
      p->windows_force_reload = 1;
    }
    else if (key == 'i')
    {
      // show/hide visibility of commands panel
      windowToggleVisibility(p->windows->w_controls);
      p->windows_force_reload = 1;
    }

    ms_sleep(SLEEP_INTERVAL);
  }

  p->ret->keypress_routine = 0;
  pthread_exit(0);
}

/**
 * @brief Checks if routine have ended
 * 
 * @param p pointer to parameters
 * @return int 
 */
int check_routines(Parameters *p)
{
  //* this is quite ugly
  return p->ret->show_routine || p->ret->advance_routine || p->ret->save_routine || p->ret->keypress_routine;
}

int main(int argc, char *argv[])
{
  // init random seed
  srand(time(NULL));

  // set raw mode
  enter_raw_mode();

  // declare variables
  pthread_t show_thread, advance_thread, save_thread, keypress_thread;
  Phase phases[3], *current_phase;
  Parameters *p;
  Windows *w;

  // init phases, provide argv and argc to handle command line parsing
  current_phase = init_phases(phases, argc, argv);
  // init windows
  w = init_windows();

  // pack the parameters
  p = init_parameters(current_phase, w);

  // prepare terminal
  clear_terminal();
  hide_cursor();

  // check if a previous file session is available
  if (check_savefile() == 1)
  {
    Dialog *d;
    int ret;

    d = createDialog(0, Y_BORDER);
    dialogSetPadding(d, 4);
    dialogSetText(d, "Previous session found. Continue?", 1);
    dialogCenter(d, 1, 0);
    dialogShow(d);
    ret = dialogWaitResponse(d);
    dialogClear(d);
    deleteDialog(d);

    // load stats from file
    if (ret)
      load_savefile(p, phases);
  }

  // handle signal interrupt
  signal(SIGINT, SIGINT_handler);
  // handle terminal resize
  signal(SIGWINCH, SIGWINCH_handler);

  // setup callback flags
  sigint_called = 0;
  sigwinch_called = 0;

  //start the loop
  p->loop = 1;

  // spawn threads
  pthread_create(&show_thread, NULL, show_routine, (void *)p);
  pthread_create(&advance_thread, NULL, advance_routine, (void *)p);
  pthread_create(&save_thread, NULL, save_routine, (void *)p);
  pthread_create(&keypress_thread, NULL, keypress_routine, (void *)p);

  reset_current_time(p);
  start_time(p);

  // Main thread IDLE
  while (check_routines(p))
  {
    ms_sleep(SLEEP_INTERVAL);
  }

  // clean up
  delete_parameters(p);

  // reset all terminal
  exit_raw_mode();
  reset_styles();
  clear_terminal();
  show_cursor();
  move_cursor_to(0, 0);

  return 0;
}