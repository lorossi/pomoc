/** @file terminal.c */

#include "terminal.h"

// Definition of private functions, so that linter and compiler are happy

double _hue_to_rgb(double p, double q, double t);
double _max(double a, double b);
double _min(double a, double b);
double _max_3(double a, double b, double c);
double _min_3(double a, double b, double c);
double _clamp(double val, double min, double max);
int _stringCopyNBytes(char *dest, char *source, int start, int end);
int _stringCopy(char *dest, char *source);
int _stringFindFirstSpace(char *s, int start);
void _stringPad(char *dest, char *source, int chars);
int _stringTrim(char *dest, char *source);
int _windowCalculateLongestLine(Window *w);
void _windowAutoWidth(Window *w, int longest);
void _windowAutoHeight(Window *w);
void _windowDrawBorder(Window *w);
int _windowCalculateSpacing(Window *w, int current_len);
int _windowLinesWrap(Window *w);
int _windowLinesUnbuffer(Window *w);
void _windowClearUnbuffered(Window *w);

/**
 * @internal
 * @brief Internal function. Used inside HSLtoRGB functions.
 * Converts hue (from HSL format) to RGB color.
 * 
 * @param p 
 * @param q 
 * @param t 
 * @return double 
 */
double _hue_to_rgb(double p, double q, double t)
{
  if (t < 0)
    t += 1;
  if (t > 1)
    t -= 1;

  if (t < 1.0 / 6)
    return p + (q - p) * 6.0 * t;
  if (t < 1.0 / 2)
    return q;
  if (t < 2.0 / 3)
    return p + (q - p) * (2.0 / 3 - t) * 6.0;

  return p;
}

/**
 * @internal
 * @brief Returns the maximum value between two.
 * 
 * @param a 
 * @param b 
 * @return double 
 */
double _max(double a, double b)
{
  if (a > b)
    return a;
  return b;
}

/**
 * @internal
 * @brief Returns the minimum value between two.
 * 
 * @param a 
 * @param b 
 * @return double 
 */
double _min(double a, double b)
{
  if (a < b)
    return a;
  return b;
}

/**
 * @internal
 * @brief Returns the maximum value between three.
 * 
 * @param a 
 * @param b 
 * @param c 
 * @return double 
 */
double _max_3(double a, double b, double c)
{
  return _max(_max(a, b), c);
}

/**
 * @internal
 * @brief Returns the minimum value between three.
 * 
 * @param a 
 * @param b 
 * @param c 
 * @return double 
 */
double _min_3(double a, double b, double c)
{
  return _min(_min(a, b), c);
}

/**
 * @internal
 * @brief Returns the length of a string.
 * Basically a redefinition o strlen.
 * 
 * @param s The string to text
 * @return int Length of the string
 */
int _stringLength(char *s)
{
  int count = 0;
  while (1)
  {
    if (s[++count] == '\0')
      return count--;
  }
}

/**
 * @internal
 * @brief Clamps a value in a certain range.
 * 
 * @param val Value to be clamped
 * @param min Minimum value of the result
 * @param max Maximum value of the result
 * @return double Clamped value
 */
double _clamp(double val, double min, double max)
{
  return _min(_max(val, min), max);
}

/**
 * @internal
 * @brief Copies N bytes o a string.
 * Basically a redefinition of strncpy.
 * The string is automatically character terminated.
 * 
 * @param dest Destination string, where the string will be saved
 * @param source Source string
 * @param start Starting character
 * @param end Ending character. If -1, end gets set to end of string.
 * @return int Number of copied characters
 */
int _stringCopyNBytes(char *dest, char *source, int start, int end)
{
  const int source_len = _stringLength(source);

  if (start < 0 || start > source_len)
    return -1;

  if (end > source_len || end == -1)
    end = source_len;

  if (start >= end)
    return -1;

  for (int i = start; i <= end; i++)
    dest[i - start] = source[i];

  if (end < source_len)
    dest[end - start + 1] = '\0';

  return end - start;
}

/**
 * @internal
 * @brief Copies a string.
 * The string is automatically character terminated.
 * 
 * @param dest Destination string, where the string will be saved
 * @param source Source string
 * @return int Number of copied characters
 */
int _stringCopy(char *dest, char *source)
{
  return _stringCopyNBytes(dest, source, 0, -1);
}

/**
 * @internal
 * @brief Finds first space in string, going backwards from end.
 * 
 * @param s String to test
 * @param start Starting position. If -1, then the starting position is the last character of the string.
 * @return int Position of the first space of -1 if no space found
 */
int _stringFindFirstSpace(char *s, int start)
{
  if (start == -1)
    start = _stringLength(s);

  if (start < 0)
    return -1;

  for (int i = start; i >= 0; i--)
  {
    if (s[i] == ' ' || s[i] == '\0')
      return i;
  }

  return -1;
}

/**
 * @internal
 * @brief Pads string with spaces.
 * 
 * @param dest Destination string, where the string will be saved
 * @param source Source string
 * @param chars NUmber of spaces to be added
 */
void _stringPad(char *dest, char *source, int chars)
{
  char buffer[MAX_WIDTH];
  const int len = _stringLength(source);

  for (int i = 0; i < chars; i++)
    buffer[i] = ' ';

  for (int i = 0; i < len; i++)
    buffer[i + chars] = source[i];

  for (int i = chars + len; i < 2 * chars + len; i++)
    buffer[i] = ' ';

  _stringCopy(dest, buffer);
}

/**
 * @internal
 * @brief Trims string, removing start and end spaces.
 * 
 * @param dest Destination string, where the string will be saved
 * @param source Source string
 * @return int Number of removed characters
 */
int _stringTrim(char *dest, char *source)
{
  int start, end;
  const int len = _stringLength(source);

  start = 0;
  end = len;

  // find actual string start
  for (int i = 0; i < len; i++)
  {
    if (source[i] != ' ')
    {
      start = i;
      break;
    }
  }

  // find actual string end
  for (int i = len - 1; i > 0; i--)
  {
    if (source[i - 1] != ' ')
    {
      end = i;
      break;
    }
  }

  int newlen;
  newlen = _stringCopyNBytes(dest, source, start, end);

  return newlen - len;
}

/**
 * @internal
 * @brief Calculates the width of a window.
 * 
 * @param w Pointer to Window to be tested
 * @return int Window external width, including border and padding
 */
int _windowCalculateLongestLine(Window *w)
{
  int longest = 0;

  for (int i = 0; i < w->lines; i++)
  {
    if (_stringLength(w->text_buffer[i]) > longest)
    {
      longest = _stringLength(w->text_buffer[i]);
    }
  }

  return longest;
}

/**
 * @internal
 * @brief Automatically sets window width.
 * 
 * @param w Pointer to Window
 * @param longest Longest line of text in the window
 */
void _windowAutoWidth(Window *w, int longest)
{
  w->size.width = longest + 2 + w->padding * 2;
  return;
}

/**
 * @internal
 * @brief Automatically sets window height.
 * 
 * @param w Pointer to Window
 */
void _windowAutoHeight(Window *w)
{
  w->size.height = w->lines + 2;
  return;
}

/**
 * @internal
 * @brief Draws window border, without clearing the window body.
 * Text and background color must be set before calling this function.
 * Terminal is automatically flushed every line.
 * 
 * @param w Pointer to Window to be drawn
 */
void _windowDrawBorder(Window *w)
{
  const int width = w->size.width;
  const int height = w->size.height;

  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      if (y == 0)
      {
        move_cursor_to(x + w->position.x, y + w->position.y);

        // top line
        if (x == 0)
          printf("\u250c"); // top left corner
        else if (x == width - 1)
          printf("\u2510"); // top right corner
        else
          printf("\u2500"); // top line
      }
      else if (y == height - 1)
      {
        move_cursor_to(x + w->position.x, y + w->position.y);

        // bottom line
        if (x == 0)
          printf("\u2514"); // bottom left corner
        else if (x == width - 1)
          printf("\u2518"); // bottom right corner
        else
          printf("\u2500"); // bottom line
      }
      else if (x == 0 || x == width - 1)
      {
        move_cursor_to(x + w->position.x, y + w->position.y);

        printf("\u2502"); // vertical line
      }
    }

    fflush(NULL);
  }
}

/**
 * @internal
 * @brief Calculates line spacing (spaces to add on the left hand side in order to
 * satisfy the current window alignment settings).
 * 
 * @param w Pointer to Window 
 * @param current_len Length of the current line
 * @return int Number of spaces to be added
 */
int _windowCalculateSpacing(Window *w, int current_len)
{
  if (w->alignment == -1)
    return 0;

  else if (w->alignment == 0)
    return (w->size.width - w->padding * 2 - 2 - current_len) / 2;

  else if (w->alignment == 1)
    return w->size.width - w->padding * 2 - 2 - current_len;

  return 0;
}

/**
 * @internal
 * @brief Wrap lines in a window.
 * 
 * @param w pointer to window
 * @return int Number of lines
 */
int _windowLinesWrap(Window *w)
{
  int lines_num = 0;
  const int width = w->size.width - 2 * w->padding - 2;
  // go over each line
  for (int i = 0; i < w->lines; i++)
  {
    _stringTrim(w->text_buffer[i], w->text_buffer[i]);
    const int len = _stringLength(w->text_buffer[i]);

    // if the line is too long
    if (len > width)
    {
      // break it into smaller parts
      int current_pos = 0;

      while (current_pos < len)
      {
        // try to look for the first break point, the closest space
        int end = _stringFindFirstSpace(w->text_buffer[i], current_pos + width);

        if (end == -1 || end < current_pos)
        {
          // if there isn't any space, take the whole line
          // make sure it's in bound of the buffer
          end = len;
        }
        else
        {
          // decrement by one to not include the space itself
          end--;
        }

        // copy to display lines
        _stringCopyNBytes(w->text[lines_num], w->text_buffer[i], current_pos, end);

        // continue to following lines
        lines_num++;
        // keep going to the char after the space
        current_pos += end - current_pos + 2;
      }
    }
    else
    {
      // simply copy into display line
      _stringCopy(w->text[lines_num], w->text_buffer[i]);
      lines_num++;
    }
  }

  // check if the window has to be resized or some lines have to be hidden
  if (lines_num > w->size.height - 2 && w->auto_height)
    w->size.height = lines_num + 2;
  else if (lines_num > w->size.height - 2)
    lines_num = w->size.height - 2;

  w->lines = lines_num;

  // copy back into display lines to prevent double wrapping
  for (int i = 0; i < w->lines; i++)
    _stringCopy(w->text_buffer[i], w->text[i]);

  return lines_num;
}

/**
 * @internal
 * @brief Copies lines from buffer to actual display.
 * 
 * @param w pointer to window
 * @return int Number of lines of text
 */
int _windowLinesUnbuffer(Window *w)
{
  for (int i = 0; i < w->lines; i++)
    _stringCopy(w->text[i], w->text_buffer[i]);

  return w->lines;
}

/**
 * @internal
 * @brief Clears a window buffer.
 * 
 * @param w pointer to window
 */
void _windowClearUnbuffered(Window *w)
{
  w->lines = 0;
}

/* 
R, G, and B are in range 0-255. */

/**
 * @brief Creates a RGB struct containing the three channels.
 * 
 * @param R Red channel, in range 0-255
 * @param G Green channel. in range 0-255
 * @param B Blue channel, in range 0-255
 * @return RGB 
 */
RGB createRGBcolor(int R, int G, int B)
{
  // clamp values just to make sure
  R = _clamp(R, 0, 255);
  G = _clamp(G, 0, 255);
  B = _clamp(B, 0, 255);
  return (RGB){.R = R, .G = G, .B = B};
}

/**
 * @brief Creates a HSL struct containing the three channels.
 * 
 * @param H Hue, in range 0-359
 * @param S Saturation, in range 0-99
 * @param L Lightness, in range 0-99
 * @return HSL 
 */
HSL createHSLcolor(int H, int S, int L)
{
  H = _clamp(H, 0, 359);
  S = _clamp(S, 0, 99);
  L = _clamp(L, 0, 99);
  return (HSL){.H = H, .S = S, .L = L};
}

/**
 * @brief Creates a Rectangle object.
 * 
 * @param w Width of the rectangle
 * @param h Height of the rectangle
 * @return Rectangle 
 */
Rectangle createRectangle(int w, int h)
{
  if (w < 0 || h < 0)
    return (Rectangle){.width = -1, .height = -1};

  return (Rectangle){.width = w, .height = h};
}

/**
 * @brief Creates a Position object.
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @return Position 
 */
Position createPosition(int x, int y)
{
  return (Position){.x = x, .y = y};
}

/**
 * @brief Creates a Window object.
 * 
 * @param x x coordinate of the top left corner
 * @param y y coordinate of the top left corner
 * @return Window* pointer to window
 */
Window *createWindow(int x, int y)
{
  Position position = createPosition(x, y);
  Rectangle size = createRectangle(1, 1);
  // allocate space for window
  Window *w = malloc(sizeof(Window));
  // pack struct
  *w = (Window){
      .auto_width = 1,
      .auto_height = 1,
      .lines = 0,
      .padding = 1,
      .alignment = -1,
      .line_wrap = 1,
      .visible = 1,
      .fg_color = fg_DEFAULT,
      .bg_color = bg_DEFAULT,
      .text_style = text_DEFAUlT,
      .size = size,
      .position = position,
  };

  return w;
}

/**
 * @brief Deletes a window, freeing the memory.
 * 
 * @param w pointer to window
 */
void deleteWindow(Window *w)
{
  if (w)
    free(w);
  return;
}

/**
 * @brief Converts a HSL color into the RGB space. 
 * 
 * @param color Color in HSL format
 * @return RGB Color in RGB format
 */
RGB HSLtoRGB(HSL color)
{
  double h, s, l;
  h = (double)color.H / 360;
  s = (double)color.S / 100;
  l = (double)color.L / 100;

  double r, g, b;
  if (s == 0)
  {
    r = l;
    g = l;
    b = l;
  }
  else
  {
    double q, p;
    q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    p = 2 * l - q;

    r = _hue_to_rgb(p, q, h + 1.0 / 3);
    g = _hue_to_rgb(p, q, h);
    b = _hue_to_rgb(p, q, h - 1.0 / 3);
  }

  return createRGBcolor(r * 255, g * 255, b * 255);
}

/**
 * @brief Converts a RGB color into the HSL space.
 * 
 * @param color Color in RGB format
 * @return HSL Color in HSL format
 */
HSL RGBtoHSL(RGB color)
{
  double r, g, b;
  r = (double)color.R / 255;
  g = (double)color.G / 255;
  b = (double)color.B / 255;

  double max, min;
  max = _max_3(r, g, b);
  min = _min_3(r, g, b);

  double h, s, l;
  l = (max + min) / 2;

  if (max == min)
  {
    h = 0;
    s = 0;
  }
  else
  {
    double d;
    d = max - min;
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

    if (max == r)
      h = (g - b) / d + (g < b ? 6 : 0);
    else if (max == g)
      h = (b - r) / d + 2;
    else if (max == b)
      h = (r - g) / d + 4;
  }

  return createHSLcolor(h * 60, s * 100, l * 100);
}

/**
 * @brief Converts hue into the RGB color space. 
 * Saturation is set to 100 and Lightness is set to 50
 * 
 * @param hue 
 * @return RGB 
 */
RGB HUEtoRGB(double hue)
{
  HSL color = createHSLcolor(hue, 100, 50);
  return HSLtoRGB(color);
}

/**
 * @brief Makes the terminal beep.
 * 
 */
void terminal_beep()
{
  fflush(NULL);
  printf(BELL);
  fflush(NULL);
}

/**
 * @brief Clears the terminal and moves cursor to upper-left position.
 * 
 */
void clear_terminal()
{
  printf(CLEARALL);
  printf(MOVEHOME);

  return;
};

/**
 * @brief Hides the blinking cursor from the terminal, disabling also echo.
 * 
 */
void hide_cursor()
{
  // turn off echo
  struct termios term;
  tcgetattr(0, &term);
  term.c_lflag &= ~ECHO;
  tcsetattr(0, TCSANOW, &term);

  printf(HIDECURSOR);
  fflush(NULL);

  return;
}

/**
 * @brief Shows echo and cursor again.
 * 
 */
void show_cursor()
{
  // turn on echo
  struct termios term;
  tcgetattr(0, &term);
  term.c_lflag |= ECHO;
  tcsetattr(0, TCSANOW, &term);

  printf(SHOWCURSOR);
  fflush(NULL);

  return;
}

/**
 * @brief Enters terminal raw mode.
 * 
 */
void enter_raw_mode()
{
  struct termios raw;
  tcgetattr(0, &raw);
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VTIME] = 0;
  raw.c_cc[VMIN] = 0;
  tcsetattr(0, TCSANOW, &raw);
  fflush(NULL);
}

/**
 * @brief Exits terminal raw mode.
 * 
 */
void exit_raw_mode()
{
  struct termios raw;
  tcgetattr(0, &raw);
  raw.c_lflag |= (ICANON | ECHO);
  raw.c_cc[VTIME] = 0;
  raw.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &raw);
  fflush(NULL);
}

/**
 * @brief Moves the cursor to the bottom of the screen.
 * 
 */
void move_cursor_to_bottom()
{
  Rectangle terminal_size = get_terminal_size();
  if (terminal_size.width != -1 && terminal_size.height != -1)
    move_cursor_to(0, terminal_size.height);

  return;
}

/**
 * @brief Returns the current size of the terminal as a rectangle. 
 * If it's not available, returns a rectangle with -1 as both dimensions.
 * 
 * @return Rectangle 
 */
Rectangle get_terminal_size()
{
  struct winsize size;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

  if (size.ws_row > 0 && size.ws_col > 0)
  {
    return createRectangle(size.ws_col, size.ws_row);
  }

  return createRectangle(-1, -1);
};

/**
 * @brief Moves cursor to x, y coordinates (zero-indexed).
 * 
 * @param x x coordinate for the cursor
 * @param y y coordinate for the cursor
 */
void move_cursor_to(int x, int y)
{
  printf(ESCAPE "[%i;%iH", y + 1, x + 1);
  fflush(NULL);

  return;
};

/**
 * @brief Resets all the previously set text styles (foreground color, background color and text modes).
 * 
 */
void reset_styles()
{
  reset_fg();
  reset_bg();
  reset_textmode();
  return;
}

/**
 * @brief Sets styles for the text. Accepts a variable number of styles.
 * The first parameter must be the number of styles.
 * 
 * @param styles The wanted styles for the terminal
 */
void set_styles(style styles, ...)
{
  va_list v;
  va_start(v, styles);

  for (int i = 0; i < styles; i++)
  {
    printf(ESCAPE "[%im", va_arg(v, style));
  }

  va_end(v);

  return;
}

/**
 * @brief Sets the color of the text.
 * 
 * @param color 
 */
void set_fg(style color)
{
  if ((color >= 30 && color <= 39) || (color >= 90 && color <= 97))
    printf(ESCAPE "[%im", color);
  return;
}

/**
 * @brief Sets the background color of the text.
 * 
 * @param color 
 */
void set_bg(style color)
{
  if ((color >= 40 && color <= 49) || (color >= 100 && color <= 107))
    printf(ESCAPE "[%im", color);
  return;
}

/**
 * @brief Sets the text mode.
 * 
 * @param mode 
 */
void set_textmode(style mode)
{
  if (mode >= 0 && mode <= 9)
    printf(ESCAPE "[%im", mode);
}

/**
 * @brief Resets the foreground color of the text. 
 * 
 */
void reset_fg()
{
  set_fg(fg_DEFAULT);
  return;
}

/**
 * @brief Resets the background color of the text.
 * 
 */
void reset_bg()
{
  set_bg(bg_DEFAULT);
  return;
}

/**
 * @brief Resets the text mode.
 * 
 */
void reset_textmode()
{
  set_textmode(text_DEFAUlT);
}

/**
 * @brief Sets the foreground color of the text, according to the RGB values.
 * 
 * @param color RGB color
 */
void set_fg_RGB(RGB color)
{
  printf(ESCAPE "[38;2;%i;%i;%im", color.R, color.G, color.B);
  return;
}

/**
 * @brief Sets the background color of the text, according to the RGB values. 
 * 
 * @param color RGB color
 */
void set_bg_RGB(RGB color)
{
  printf(ESCAPE "[48;2;%i;%i;%im", color.R, color.G, color.B);
  return;
}

/**
 * @brief Sets the foreground color of the text, according to the HSL values.
 * 
 * @param color HSL color
 */
void set_fg_HSL(HSL color)
{
  RGB converted = HSLtoRGB(color);
  set_fg_RGB(converted);
}

/**
 * @brief Sets the background color of the text, according to the HSL values. 
 * 
 * @param color HSL color
 */
void set_bg_HSL(HSL color)
{
  RGB converted = HSLtoRGB(color);
  set_bg_RGB(converted);
}

/**
 * @brief Writes a string at certain x,y coordinates (zero-indexed).
 * 
 * @param x x coordinate of the text
 * @param y y coordinate of the text
 * @param s string to print
 */
void write_at(int x, int y, char *s)
{
  move_cursor_to(x, y);
  printf(s);

  return;
};

/**
 * @brief Erases a set number of characters at certain x,y coordinates (zero-indexed).
 * 
 * @param x x coordinate of the first character to delete
 * @param y y coordinate of the first character to delete
 * @param length number of characters to delete
 */
void erase_at(int x, int y, int length)
{
  for (int i = 0; i < length; i++)
  {
    move_cursor_to(x + i, y);
    printf(" ");
  }

  return;
}

/**
 * @brief Polls a keypress. Returns the code corresponding to the key. Needs to be in raw mode.
 * This function does not wait for enter and does not work with special characters.
 * 
 * @return char 
 */
char poll_keypress()
{
  char buf;

  if (read(0, &buf, 1) == 0)
    buf = 0;

  return buf;
}

/**
 * @brief Polls special key presses.Needs to be in raw mode. 
 * 
 * @return char Return value: the 8 lsb represent (from left to right): BACKSPACE SPACEBAR ENTER TAB RIGHT LEFT DOWN UP
 */
char poll_special_keypress()
{
  char key;
  char pressed, status;
  // to catch arrrow keypressed we need to read 3 characters
  // 2 delimiters and 1 for the actual button
  // implemented as a simple FSM
  // the 8 lsb represent (from left to right)
  // BACKSPACE SPACEBAR ENTER TAB RIGHT LEFT DOWN UP pressed.
  pressed = 0;
  status = 0;
  while ((key = poll_keypress()) > 0)
  {
    if (key == 27)
    {
      // first step of the FSM
      // first escape character found
      status |= 0b00000001;
    }
    else if (key == 91 && status & 0b00000001)
    {
      // second step of the FSM
      // second escape character found
      status |= 0b00000010;
    }
    else if (key >= 65 && key <= 68 && status & 0b00000010)
    {
      // last step of the FSM
      // key character found
      switch (key)
      {
      case 65:
        pressed |= 0b00000001; // up
        break;
      case 66:
        pressed |= 0b00000010; // down
        break;
      case 67:
        pressed |= 0b00000100; // left
        break;
      case 68:
        pressed |= 0b00001000; // right
        break;

      default:
        break;
      }

      pressed &= 0b00001111;
      status &= 0b11110000;
    }
    else
    {
      // some keys are not escaped
      status = 0;
      switch (key)
      {
      case 9:
        pressed |= 0b00010000; // tab
        break;
      case 10:
        pressed |= 0b00100000; // enter
        break;
      case 32:
        pressed |= 0b01000000; // space bar
        break;
      case 127:
        pressed |= 0b10000000; // backspace
        break;
      default:
        break;
      }
    }

    // if any key was found, return
    if (pressed)
      break;
  }

  return pressed;
}

/**
 * @brief Awaits a keypress. A message is prompted on the terminal. Needs to be in raw mode.
 * 
 * @param s String to print, pass NULL to skip
 * @return int Code of the pressed key
 */
int await_keypress(char *s)
{
  if (s != NULL)
    printf("%s", s);

  int read_bytes;
  char buffer[1];

  do
  {
    read_bytes = read(0, buffer, 1);
  } while (read_bytes == 0);

  fflush(NULL);

  return (int)buffer[0];
}

/**
 * @brief Awaits a enter keypress. A message is prompted on the terminal. Does not work within raw mode.
 * 
 * @param s tring to print, pass NULL to skip
 * @return int Code of the pressed key
 */
int await_enter(char *s)
{
  if (s != NULL)
    printf("%s", s);

  return getchar();
}

/**
 * @brief  Sets size of a window. Size is relative to the outer border. 
 * 
 * @param w pointer to window
 * @param width desired width of the window
 * @param height desired height of the window
 */
void windowSetSize(Window *w, int width, int height)
{
  if (width < 0 || height < 0)
    return;

  if (width != w->size.width || height != w->size.height)
    w->size = createRectangle(width, height);

  return;
}

/**
 * @brief Sets width of a window. Size is relative to the outer border.
 * 
 * @param w pointer to window
 * @param width desired width of the window
 */
void windowSetWidth(Window *w, int width)
{
  if (width < 0)
    return;

  if (width != w->size.width)
  {
    windowClear(w);
    w->size.width = width;
  }

  return;
}

/**
 * @brief Sets height of a window. Size is relative to the outer border.
 * 
 * @param w pointer to window
 * @param height desired height of the window
 */
void windowSetHeight(Window *w, int height)
{
  if (height < 0)
    return;

  if (height != w->size.height)
  {
    windowClear(w);
    w->size.height = height;
  }

  return;
}

/**
 * @brief Sets visibility of a window.
 * 
 * @param w pointer to window
 * @param visibility 1 to make the widow visible, 0 to hide it
 */
void windowSetVisibility(Window *w, int visibility)
{
  if (visibility != 0 && visibility != 1)
    return;

  w->visible = visibility;
}

/**
 * @brief Toggles a visibility of a window
 * 
 * @param w pointer to window
 * @return int current visibility status (0: invisibile, 1: visible)
 */
int windowToggleVisibility(Window *w)
{
  w->visible = !w->visible;

  return w->visible;
}

/**
 * @brief Returns window size, if visibile. Otherwise, returns an empty rectangle.
 * 
 * @param w pointer to window
 * @return Rectangle 
 */
Rectangle windowGetSize(Window *w)
{
  if (w->visible)
    return w->size;

  return createRectangle(0, 0);
}

/**
 * @brief Sets position of a window, relative to top left corner.
 * 
 * @param w pointer to window
 * @param x x coordinate (-1 to keep the same coordinate)
 * @param y y coordinate (-1 to keep the same coordinate)
 */
void windowSetPosition(Window *w, int x, int y)
{
  const Position old_position = windowGetPosition(w);
  Position new_position = (Position){.x = x, .y = y};

  if (x == -1)
    new_position.x = old_position.x;
  if (y == -1)
    new_position.y = old_position.y;

  w->position = createPosition(new_position.x, new_position.y);
  return;
}

/**
 * @brief Move window by a set amount of characters
 * 
 * @param w pointer to window
 * @param dx displacement along x axis
 * @param dy displacement along y axis
 */
void windowMoveBy(Window *w, int dx, int dy)
{
  w->position.x += dx;
  w->position.y += dy;
}

/**
 * @brief Gets the position of a window.
 * 
 * @param w pointer to window
 * @return Position position of top left corner
 */
Position windowGetPosition(Window *w)
{
  return w->position;
}

/**
 * @brief  Gets the position of the bottom right corner of the window.
 * 
 * @param w pointer to window
 * @return Position position of the bottom right corner
 */
Position windowGetBottomRight(Window *w)
{
  return (Position){
      .x = w->position.x + w->size.width,
      .y = w->position.y + w->size.height,
  };
}

/**
 * @brief Sets window padding.
 * 
 * @param w pointer to window
 * @param padding left and right padding between text and border
 */
void windowSetPadding(Window *w, int padding)
{
  if (padding > 0)
    w->padding = padding;

  return;
}

/**
 * @brief Sets window text alignment.
 * 
 * @param w pointer to window
 * @param alignment  -1 for left, 0 for center, 1 for right
 */
void windowSetAlignment(Window *w, int alignment)
{
  if (-1 <= alignment && alignment <= 1)
    w->alignment = alignment;

  return;
}

/**
 * @brief Sets window auto size.
 * 
 * @param w pointer to window
 * @param auto_size 1 for automatic sizing, 0 for manual sizing
 */
void windowSetAutoSize(Window *w, int auto_size)
{
  if (auto_size != 0 && auto_size != 1)
    return;

  w->auto_height = auto_size;
  w->auto_width = auto_size;
}

/**
 * @brief Sets window auto width. 
 * 
 * @param w pointer to window
 * @param auto_size 1 for automatic width, 0 for manual sizing.
 */
void windowSetAutoWidth(Window *w, int auto_size)
{
  if (auto_size != 0 && auto_size != 1)
    return;

  w->auto_width = auto_size;
}

/**
 * @brief Sets window auto height.
 * 
 * @param w pointer to window
 * @param auto_size 1 for automatic height, 0 for manual sizing.
 */
void windowSetAutoHeight(Window *w, int auto_size)
{
  if (auto_size != 0 && auto_size != 1)
    return;

  w->auto_height = auto_size;
}

/**
 * @brief Sets window line wrap.
 * 
 * @param w pointer to window
 * @param line_wrap 1 for automatic wrapping, 0 for no wrapping
 */
void windowSetLineWrap(Window *w, int line_wrap)
{
  if (line_wrap != 0 && line_wrap != 1)
    return;

  w->line_wrap = line_wrap;
}

/**
 * @brief Manually triggers window resize.
 * 
 * @param w pointer to window
 */
void windowAutoResize(Window *w)
{
  if (w->auto_width)
  {
    int longest;
    longest = _windowCalculateLongestLine(w);
    _windowAutoWidth(w, longest);
  }

  if (w->auto_height)
  {
    _windowLinesWrap(w);
    _windowAutoHeight(w);
  }
}

/**
 * @brief Sets window text color.
 * 
 * @param w pointer to window
 * @param fg_color text style color
 */
void windowSetFGcolor(Window *w, style fg_color)
{
  w->fg_color = fg_color;
  return;
}

/**
 * @brief Sets window background color.
 * 
 * @param w pointer to window
 * @param bg_color text background style color
 */
void windowSetBGcolor(Window *w, style bg_color)
{
  w->bg_color = bg_color;
  return;
}

/**
 * @brief Sets window text style.
 * 
 * @param w pointer to window
 * @param textstyle text style
 */
void windowSetTextStyle(Window *w, style textstyle)
{
  w->text_style = textstyle;
  return;
}

/**
 * @brief Returns the number of line of text of a window.
 * 
 * @param w pointer to window
 * @return int number of linesin the window
 */
int windowGetLines(Window *w)
{
  return w->lines;
}

/**
 * @brief Adds a line of text to the window.
 * 
 * @param w pointer to window
 * @param line line to add
 * @return int -1 in case of error, otherwise length of the line
 */
int windowAddLine(Window *w, char *line)
{
  if (w->lines > MAX_LINES)
    return -1;

  _stringCopy(w->text_buffer[w->lines], line);
  w->lines++;

  return sizeof(w->text_buffer[w->lines - 1]);
}

/**
 * @brief  Changes a line of text in the window.
 * 
 * @param w pointer to window
 * @param line new content of the line
 * @param line_count index of the line to modify
 * @return int -1 in case of error, otherwise length of the line
 */
int windowChangeLine(Window *w, char *line, int line_count)
{
  if (line_count > w->lines)
    return -1;

  _stringCopy(w->text_buffer[line_count], line);

  return sizeof(w->text_buffer[line_count]);
}

/**
 * @brief Deletes a line of text in the window.
 * 
 * @param w pointer to window
 * @param line_index index of the line to remove
 * @return int -1 in case of error, otherwise the new number of lines
 */
int windowDeleteLine(Window *w, int line_index)
{
  if (line_index >= w->lines)
    return -1;

  w->lines--;

  for (int i = line_index; i < w->lines; i++)
    _stringCopy(w->text_buffer[i], w->text_buffer[i + 1]);

  return w->lines;
}

/**
 * @brief Deletes a all the lines of text in the window.
 * 
 * @param w pointer to window
 * @return int -1 in case of error, 0 otherwise
 */
int windowDeleteAllLines(Window *w)
{
  w->lines = 0;
  return 0;
}

/**
 * @brief Shows a window on the terminal.
 * 
 * @param w pointer to window
 */
void windowShow(Window *w)
{
  // hidden, return
  if (!w->visible)
    return;

  int longest;
  // calculate longest line
  longest = _windowCalculateLongestLine(w);
  // auto resize window
  if (w->auto_width)
    _windowAutoWidth(w, longest);
  if (w->auto_height)
    _windowAutoHeight(w);

  // check if lines need to be wrapped
  const int width = w->size.width - 2 * w->padding;
  if (longest >= width && w->line_wrap)
  {
    // windows are auto resized if needed
    _windowLinesWrap(w);
    // re calculate longest line
    longest = _windowCalculateLongestLine(w);
  }
  else
  {
    // simply copy the buffer onto the screen
    _windowLinesUnbuffer(w);
  }

  // set styles
  set_fg(w->fg_color);
  set_bg(w->bg_color);

  // draw outer border
  _windowDrawBorder(w);

  // set text style
  // prevent complete reset
  if (w->text_style != text_DEFAUlT)
    set_textmode(w->text_style);

  for (int i = 0; i < w->lines; i++)
  {
    // calculate line length
    const int ll = _stringLength(w->text[i]);
    // calculate spacing according to alignment
    int spacing = _windowCalculateSpacing(w, ll);
    // calculate line coordinates
    const int lx = w->position.x + w->padding + spacing + 1;
    const int ly = w->position.y + i + 1;
    // draw text
    move_cursor_to(lx, ly);
    printf("%s", w->text[i]);
    fflush(NULL);
  }

  reset_styles();
  move_cursor_to(0, 0);
}

/**
 * @brief Clears a window from terminal.
 * 
 * @param w pointer to window
 */
void windowClear(Window *w)
{
  reset_bg();
  for (int y = -1; y < w->size.height + 1; y++)
    erase_at(w->position.x, y + w->position.y, w->size.width);
}

/**
 * @brief Create a dialog window.
 * 
 * @param x x position of the dialog
 * @param y y position of the image
 * @return Dialog* 
 */
Dialog *createDialog(int x, int y)
{
  int width, height;
  // calculate size TODO IMPROVE
  width = DIALOG_MAX_WIDTH;
  height = DIALOG_MAX_HEIGHT;
  // calculate border
  // create a window
  Window *w = createWindow(x, y);
  // manual size it
  windowSetSize(w, width, height);
  windowSetAutoSize(w, 0);

  // create buttons
  Window *b1, *b2;
  b1 = createWindow(x + 4, y - 4 + height);
  b2 = createWindow(x - 11 - 4 + width, y - 4 + height);
  // set alignment
  windowSetAlignment(b1, 0);
  windowSetAlignment(b2, 0);
  windowSetAlignment(w, 0);
  // set padding
  windowSetPadding(b1, 0);
  windowSetPadding(b2, 0);
  // set text
  windowDeleteAllLines(b1);
  windowAddLine(b1, "  NO  ");
  windowDeleteAllLines(b2);
  windowAddLine(b2, "  YES  ");

  // allocate space for dialog
  Dialog *d = malloc(sizeof(Window));
  // pack struct
  *d = (Dialog){
      .active_button = 0,
      .center_x = 0,
      .center_y = 0,
      .window = w,
      .buttons = {b1, b2},
  };

  return d;
}

/**
 * @brief Deletes a dialog window.
 * 
 * @param d pointer to dialog
 */
void deleteDialog(Dialog *d)
{
  // free window
  deleteWindow(d->window);
  // free buttons
  for (int i = 0; i < 2; i++)
    deleteWindow(d->buttons[i]);
  free(d);
}

/**
 * @brief Centers a dialog window in the terminal
 * 
 * @param d Pointer to dialog
 * @param center_x 1 to align on the x axis, 0 to leave as it is
 * @param center_y 1 to align on the y axis, 0 to leave as it is
 */
void dialogCenter(Dialog *d, int center_x, int center_y)
{
  if (center_x != 0 && center_x != 1)
    return;
  if (center_y != 0 && center_y != 1)
    return;

  d->center_x = center_x;
  d->center_y = center_y;
}

/**
 * @brief Shows a dialog window
 * 
 * @param d pointer to dialog window
 */
void dialogShow(Dialog *d)
{

  if (d->center_x || d->center_y)
  {
    // get current dialog position
    Position dialog_position = windowGetPosition(d->window);
    // get size of the terminal
    Rectangle terminal_size = get_terminal_size();
    // get size of dialog window
    Rectangle dialog_size = windowGetSize(d->window);

    if (terminal_size.height != -1 || terminal_size.width != -1)
    {
      // calculate x and y variation in position
      if (d->center_x)
      {
        int dx;
        // calculate horizontal displacement
        dx = (terminal_size.width - dialog_size.width) / 2 - dialog_position.x;
        // set position for main window
        d->window->position.x += dx;
        // set position for buttons
        for (int i = 0; i < 2; i++)
          d->buttons[i]->position.x += dx;
      }

      if (d->center_y)
      {
        int dy;
        // calculate vertical displacement
        dy = (terminal_size.height - dialog_size.height) / 2 - dialog_position.y;
        // set position for main window
        d->window->position.y += dy;
        // set position for buttons
        for (int i = 0; i < 2; i++)
          d->buttons[i]->position.y += dy;
      }
    }
  }

  windowShow(d->window);

  for (int i = 0; i < 2; i++)
  {
    windowSetTextStyle(d->buttons[i], i == d->active_button ? text_REVERSE : text_DEFAUlT);
    windowShow(d->buttons[i]);
  }
}

/**
 * @brief Hides a dialog window
 * 
 * @param d pointer to dialog
 */
void dialogClear(Dialog *d)
{
  windowClear(d->window);

  for (int i = 0; i < 2; i++)
    windowClear(d->buttons[i]);
}

/**
 * @brief Set buttons for the dialog
 * 
 * @param d pointer to dialog window
 * @param yes string of the first button
 * @param no string of the second button
 */
void dialogSetButtons(Dialog *d, char *yes, char *no)
{
  char buffer[MAX_WIDTH];
  // clear old lines
  windowDeleteAllLines(d->buttons[0]);
  windowDeleteAllLines(d->buttons[1]);
  // add new lines
  _stringCopy(buffer, yes);
  _stringPad(buffer, buffer, 2);
  windowAddLine(d->buttons[0], buffer);
  _stringCopy(buffer, no);
  _stringPad(buffer, buffer, 2);
  windowAddLine(d->buttons[1], buffer);
  // trigger resize
  windowAutoResize(d->buttons[1]);
  // get bottom right of the window
  const Position bottom_right = windowGetBottomRight(d->window);
  // move right button
  const Rectangle button_width = windowGetSize(d->buttons[1]);
  // calculate x coordinate of the button
  const int x = bottom_right.x - 4 - button_width.width;
  // set x coord of the button
  windowSetPosition(d->buttons[1], x, -1);
}

/**
 * @brief Sets dialog horizontal padding
 * 
 * @param d pointer to dialog window
 * @param padding horizontal padding
 */
void dialogSetPadding(Dialog *d, int padding)
{
  if (padding > 0)
    d->window->padding = padding;

  return;
}

/**
 * @brief Sets text for the dialog
 * 
 * @param d pointer to dialog window
 * @param text text to be put on the dialog
 * @param v_padding vertical padding
 */
void dialogSetText(Dialog *d, char *text, int v_padding)
{
  windowDeleteAllLines(d->window);

  for (int i = 0; i < v_padding; i++)
    windowAddLine(d->window, "");

  windowAddLine(d->window, text);
  return;
}

/* Polls a response for for the dialog. Needs raw mode. */
/**
 * @brief Polls a response for the dialog. Needs to be in dialog mode
 * 
 * @param d pointer to dialog
 * @return int returns the id of the pressed button
 */
int dialogWaitResponse(Dialog *d)
{
  while (1)
  {

    int special = poll_special_keypress();

    if (special & 0b00000100) // left
      d->active_button = 1;
    else if (special & 0b00001000) // right
      d->active_button = 0;
    else if (special & 0b00010000) // tab
      d->active_button = !d->active_button;
    else if (special & 0b00100000) // enter
      return d->active_button;

    if (special)
      dialogShow(d);
  }
}