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
XColor bg_color, border_color;

int error_handler(Display *disp, XErrorEvent *xe) {
  switch(xe->error_code) {
    case BadAccess:
      printf("popcorn: [BadAccess] Cant grab key binding. Already grabbed\n");
      return 0;
  }

  printf("popcorn: Something went wrong\n");
  return 1;
}

XColor to_xcolor(const char *colorstr) {
  XColor dummy, ptr;
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), colorstr, &ptr, &dummy);
	return ptr;
}

void draw_stuff() {
  Window root_win;
  XSetWindowAttributes wa;

  wa.override_redirect = 1;
	wa.background_pixel = bg_color.pixel;
	wa.border_pixel = border_color.pixel;

  win = XCreateWindow(dpy, root,
      x, y,
      width, height,
      0, DefaultDepth(dpy, screen),
      CopyFromParent,
      DefaultVisual(dpy, screen),
      CWOverrideRedirect | CWBackPixel | CWBorderPixel, &wa);

  XSetWindowBorderWidth(dpy, win, border_width);

  XGCValues gr_values;
  gr_values.foreground = CWBackPixel;
  gr_values.background = CWBackPixel;
  GC gc = XCreateGC(dpy, win, GCForeground | GCBackground, &gr_values);

  XFillRectangle(dpy, win, gc, 0, 0, 20, 20);
  XFreeGC(dpy, gc);

  XMapRaised(dpy, win);
}

void initialize_values() {
  int i, s_dimen;

	border_color = to_xcolor(border);
	bg_color = to_xcolor(background);

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

  int running = 1;

  dpy = XOpenDisplay(0);
  root = RootWindow(dpy, screen);

  initialize_values();

  draw_stuff();

  /* main event loop */
  XEvent ev;
	XSync(dpy, 0);

  while (running && XNextEvent(dpy, &ev)) {
    switch (ev.type) {
      case Expose:
        printf("Expose");
        break;
      case VisibilityNotify:
        printf("Visible");
        break;
      case KeyPress: {
        break;
      }
    }
  }

  XCloseDisplay(dpy);
}

