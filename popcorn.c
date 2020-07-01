#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "config.h"

#define LENGTH(X) (sizeof X / sizeof X[0])
#define CLEANMASK(mask) (mask & ~LockMask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

#define MAX_STRING_SIZE 500

static Display *dpy;
static Window root, win;
static int screen;
XftColor bg_color, border_color, fg_color;
XftFont * fontset[5];

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
  XftColor dummy, ptr;
	XftColorAllocName(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen),
	    colorstr, &ptr);
	return ptr;
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
  XftDrawStringUtf8(xftdraw, &fg_color, fontset[0], padding_left, padding_top, (XftChar8 *) text, len);

  if (xftdraw) {
		XftDrawDestroy(xftdraw);
  }
}

void draw_popup() {
  Window root_win;
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

void initialize_values() {
  int i, s_dimen;

	border_color = to_xftcolor(border);
	bg_color = to_xftcolor(background);
	fg_color = to_xftcolor(foreground);

	XftFont * font = NULL;

  for (int i = 0; i < LENGTH(fonts); i++) {
    if(!(fontset[i] = XftFontOpenName(dpy, screen, fonts[i]))) {
      fprintf(stderr, "error, cannot load font from name: '%s'\n", fonts[i]);
      exit(1);
    }
  }

	// TODO: Calculate auto height

	if (x < 0) {
    s_dimen = DisplayWidth(dpy, screen);
    x = s_dimen - width + x;
  }

	if (y < 0) {
    s_dimen = DisplayHeight(dpy, screen);
    y = s_dimen - height + y;
  }
}

int main() {
  XSetErrorHandler(error_handler);

  int running = 2;

  dpy = XOpenDisplay(0);
  root = RootWindow(dpy, screen);

  /*sigchld(0);*/
  initialize_values();

  draw_popup();

  /* main event loop */
  XEvent ev;
	XSync(dpy, 0);

  while (running) {
    XNextEvent(dpy, &ev);

    switch (ev.type) {
      case Expose:
        draw_popup_text("Hello world");
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

