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

#define MAX_TEXT_SIZE 500

#include "config.h"

#define LENGTH(X) (sizeof X / sizeof X[0])
#define CLEANMASK(mask) (mask & ~LockMask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

Display *dpy;
Window root, win;
int screen;
int content_width, content_height;
XftColor bg_color, border_color, fg_color;
XftFont* fontset[5];
pthread_t reader_thread;

int auto_height = 0;

char text[MAX_TEXT_SIZE];

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

int get_textwidth(const char* text, unsigned int len) {
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
	  // TODO: Case when a word is too big, currently it just overflows
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

void recalculate() {
  int s_dimen, len = strlen(text);
  content_width = width - padding_left - padding_right;
  int lines;

  if (win) {
    lines = word_wrap(text, len, content_width);

    if (auto_height) {
      height = (lines * line_height) + padding_top + padding_bottom;
      XWindowChanges props;
      props.height = height;
      XConfigureWindow(dpy, win, CWHeight, &props);
    }
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

void draw_popup_text() {
  if (!win) return;
  recalculate();

  XftDraw* xftdraw = XftDrawCreate(dpy, win,
      DefaultVisual(dpy, screen),
      DefaultColormap(dpy, screen));

  int len = strlen(text);

  char buffer[len + 1];
  int bufflen = 0;

  int tx = padding_left;
  int ty = padding_top;
  int content_height = 0;

  ty += fontset[0]->ascent;
  for (int i = 0; i <= len; i++) {
    if (text[i] == '\n' || text[i] == '\0') {
      XftDrawStringUtf8(xftdraw, &fg_color, fontset[0], tx, ty, (XftChar8 *) buffer, bufflen);
      ty += line_height;
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

void setup() {
  int i;

	fg_color = to_xftcolor(foreground);
	bg_color = to_xftcolor(background);
	border_color = to_xftcolor(border);

  for (i = 0; i < LENGTH(fonts); i++) {
    if(!(fontset[i] = XftFontOpenName(dpy, screen, fonts[i]))) {
      fprintf(stderr, "error, cannot load font from name: '%s'\n", fonts[i]);
      exit(1);
    }
  }

  auto_height = height == 0;

  recalculate();
}

void create_popup_window() {
  XSetWindowAttributes wa;

  wa.override_redirect = 1;
	wa.background_pixel = bg_color.pixel;
	wa.border_pixel = border_color.pixel;
	wa.event_mask = ExposureMask | VisibilityChangeMask | KeyPressMask;

  win = XCreateWindow(dpy, root,
      x, y,
      width, auto_height ? line_height : height,
      0, DefaultDepth(dpy, screen),
      CopyFromParent,
      DefaultVisual(dpy, screen),
      CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask, &wa);

  XSelectInput(dpy, root, wa.event_mask);
  XSetWindowBorderWidth(dpy, win, border_width);

  XMapRaised(dpy, win);
}

void* input_reader() {
  char buf[MAX_TEXT_SIZE];

  while (fgets(buf, sizeof buf, stdin)) {
    strcat(text, buf);
    /*draw_popup_text();*/
  }

  exit(0);
}

void read_cli_args(int argc, char** argv) {
  for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			puts("popcorn-"VERSION);
			exit(0);
    } else if (!strcmp(argv[i],        "--fg")) {
      foreground = argv[++i];
    } else if (!strcmp(argv[i],        "--bg")) {
      background = argv[++i];
    } else if (!strcmp(argv[i],        "--border-color")) {
      border = argv[++i];
    } else if (!strcmp(argv[i],        "--border-size")) {
      border_width = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "-x")) {
      x = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "-y")) {
      y = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--width")) {
      width = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--height")) {
      height = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--padding-top")) {
      padding_top = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--padding-bottom")) {
      padding_bottom = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--padding-left")) {
      padding_left = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--padding-right")) {
      padding_right = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--line-height")) {
      padding_right = atoi(argv[++i]);
    } else if (!strcmp(argv[i],        "--font")) {
      fonts[0] = argv[++i];
    } else {
      fprintf(stderr, "Invalid arg: %s\n", argv[i]);
      exit(1);
    }
  }
}

int main(int argc, char** argv) {
  /*XSetErrorHandler(error_handler);*/
  read_cli_args(argc, argv);
  pthread_create(&reader_thread, NULL, input_reader, 0);

  dpy = XOpenDisplay(0);
  root = RootWindow(dpy, screen);

  setup();

  create_popup_window();

  /* main event loop */
  XEvent ev;
	XSync(dpy, 0);

  while (1) {
    XNextEvent(dpy, &ev);

    switch (ev.type) {
      case Expose:
        draw_popup_text();
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

