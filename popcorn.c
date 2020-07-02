#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "config.h"

#define LENGTH(X) (sizeof X / sizeof X[0])
#define CLEANMASK(mask) (mask & ~LockMask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

#define MAX_STRING_SIZE 500

Display *dpy;
Window root, win;
int screen;
int content_width, content_height;
XftColor bg_color, border_color, fg_color;
XftFont * fontset[5];

char text[] = "one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen twenty twentyone twentytwo twentythree";

int error_handler(Display *disp, XErrorEvent *xe) {
  switch(xe->error_code) {
    case BadAccess:
      printf("popcorn: [BadAccess] Cant grab key binding. Already grabbed\n");
      return 0;
  }

  printf("popcorn: Something went wrong\n");
  return 1;
}

XftColor to_xftcolor(const char *colorstr) {
  XftColor ptr;
	XftColorAllocName(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen),
	    colorstr, &ptr);
	return ptr;
}

int get_textwidth(const char * text, unsigned int len) {
	XGlyphInfo ext;
	XftTextExtentsUtf8(dpy, fontset[0], (XftChar8*) text, len, &ext);
	return ext.xOff;
}

int word_wrap(char* text, int length, int wrap_width) {
	int i;
	char buffer[length + 1];

  int lines_count = 0, bufflength = 0;
  int width, previous_space = 0;

	for(i = 0; i <= length; i++) {
	  // TODO: Case when a word is too big
	  switch(text[i]) {
      case ' ':
      case '\0':
        width = get_textwidth(buffer, bufflength);

        /*printf("%s\n", buffer);*/

        if (width > wrap_width) {
          lines_count++;

          if (previous_space != 0) {
            text[previous_space] = '\n';
            for (bufflength = 0; bufflength < i - previous_space; bufflength++) {
              buffer[bufflength] = text[bufflength + previous_space];
            }
          } else {
            text[i] = '\n';
            buffer[0] = '\0';
            bufflength = 0;
          }

          previous_space = 0;
          break;
        } else {
          previous_space = i;
        }

        buffer[bufflength++] = text[i];
        buffer[bufflength] = '\0';

        break;
      case '\n':
        lines_count++;
        buffer[0] = '\0';
        bufflength = 0;
        previous_space = 0;
        break;
      default:
        buffer[bufflength++] = text[i];
        buffer[bufflength] = '\0';
    }
  }

  return lines_count + (strlen(buffer) > 0);
}

void draw_popup_text(char * text) {
  int len;

  XftDraw * xftdraw = XftDrawCreate(dpy, win,
      DefaultVisual(dpy, screen),
      DefaultColormap(dpy, screen));

  XGCValues gr_values;
  gr_values.foreground = fg_color.pixel;
  gr_values.background = bg_color.pixel;
  GC gc = XCreateGC(dpy, win, GCForeground|GCBackground, &gr_values);

	XSetForeground(dpy, gc, fg_color.pixel);
	XSetBackground(dpy, gc, fg_color.pixel);

  len = strlen(text);

  char buffer[len];
  int bufflen = 0;

  int x = padding_left;
  int y = padding_top;
  int content_height = 0;

  y += fontset[0]->ascent;
  for (int i = 0; i <= len; i++) {
    if (text[i] == '\n' || text[i] == '\0') {
      XftDrawStringUtf8(xftdraw, &fg_color, fontset[0], x, y, (XftChar8 *) buffer, bufflen);
      y += line_height;
      content_height += line_height;
      buffer[0] = '\0';
      bufflen = 0;
      continue;
    }

    buffer[bufflen++] = text[i];
    buffer[bufflen] = '\0';
  }

  if (xftdraw) {
		XftDrawDestroy(xftdraw);
  }
}

void initialize_values() {
  int i, s_dimen;

	border_color = to_xftcolor(border);
	bg_color = to_xftcolor(background);
	fg_color = to_xftcolor(foreground);

  for (i = 0; i < LENGTH(fonts); i++) {
    if(!(fontset[i] = XftFontOpenName(dpy, screen, fonts[i]))) {
      fprintf(stderr, "error, cannot load font from name: '%s'\n", fonts[i]);
      exit(1);
    }
  }

  int len = strlen(text);
  content_width = width - padding_left - padding_right;
  int lines = word_wrap(text, len, content_width);

  if (height == 0) {
    height = (lines * line_height) + padding_top + padding_bottom;
  }

	if (x < 0) {
    s_dimen = DisplayWidth(dpy, screen);
    x = s_dimen - width + x;
  }

	if (y < 0) {
    s_dimen = DisplayHeight(dpy, screen);
    y = s_dimen - height + y;
  }
}

void draw_popup() {
  XSetWindowAttributes wa;

  wa.override_redirect = 1;
	wa.background_pixel = bg_color.pixel;
	wa.border_pixel = border_color.pixel;
	wa.event_mask = ExposureMask | VisibilityChangeMask | KeyPressMask;

  win = XCreateWindow(dpy, root,
      x, y,
      width, height,
      0, DefaultDepth(dpy, screen),
      CopyFromParent,
      DefaultVisual(dpy, screen),
      CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask, &wa);

  XSelectInput(dpy, root, wa.event_mask);
  XSetWindowBorderWidth(dpy, win, border_width);

  XMapRaised(dpy, win);
}

int main() {
  XSetErrorHandler(error_handler);

  int running = 2;

  dpy = XOpenDisplay(0);
  root = RootWindow(dpy, screen);

  initialize_values();

  draw_popup();

  /* main event loop */
  XEvent ev;
	XSync(dpy, 0);

  while (running) {
    XNextEvent(dpy, &ev);

    switch (ev.type) {
      case Expose:
        draw_popup_text(text);
        break;
      case VisibilityNotify:
        break;
      case KeyPress: {
        break;
      }
    }
  }

  XCloseDisplay(dpy);
}

