#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

#include "util.h"
#include "drw.h"
#include "config.h"

#define LENGTH(X) (sizeof X / sizeof X[0])
#define CLEANMASK(mask) (mask & ~LockMask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

#define MAX_STRING_SIZE 500

extern char** environ;

void bind_key(Display *dpy, Window win, unsigned int mod, KeySym key) {
  int keycode = XKeysymToKeycode(dpy, key);
  XGrabKey(dpy, keycode, mod, win, False, GrabModeAsync, GrabModeAsync);
}

int error_handler(Display *disp, XErrorEvent *xe) {
  switch(xe->error_code) {
    case BadAccess:
      printf("popcorn: [BadAccess] Cant grab key binding. Already grabbed\n");
      return 0;
  }

  printf("popcorn: Something went wrong\n");
  return 1;
}

void spawn(char** command) {
  if (fork() == 0) {
    setsid();
    execve(command[0], command, environ);
    fprintf(stderr, "popcorn: execve %s", command[0]);
    perror(" failed");
    exit(0);
  }
}

// void keypress(Display *dpy, Window win, XKeyEvent *ev) {
//   unsigned int i, stay_in_mode = False;
//   KeySym keysym = XKeycodeToKeysym(dpy, (KeyCode) ev->keycode, 0);
// 
//   // Bind all the normal mode keys
//   for (i = 0; i < LENGTH(keys); i++) {
//     if (keysym == keys[i].key && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)) {
//       printf("Keypress");
//     }
//   }
// }

static Display *dpy;
static Window root, win;
static Visual *visual;
static int mon = -1, screen;
static int depth;
static Colormap cmap;
static Drw *drw;

int x = 10;
int y = 10;
int mw = 100;
int mh = 100;
int border_width = 1;
int usergb = 0;

char text[500];

static void
cleanup(void)
{
	size_t i;

	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	/*for (i = 0; i < SchemeLast; i++)*/
		/*free(scheme[i]);*/
	drw_free(drw);
	XSync(dpy, False);
	XCloseDisplay(dpy);
}

static void*
readstdin(void * runningptr)
{
  int *running = (int *) runningptr;

	char buf[MAX_STRING_SIZE], *p;
	size_t i, imax = 0, size = 0;
	unsigned int tmpmax = 0;

	/* read each line from stdin and add it to the item list */
	for (i = 0; fgets(buf, sizeof buf, stdin); i++) {
	  strcat(text, buf);
	}

	printf("Hello %s\n", text);
  *running = 0;
}


static void
xinitvisual()
{
	XVisualInfo *infos;
	XRenderPictFormat *fmt;
	int nitems;
	int i;

	XVisualInfo tpl = {
		.screen = screen,
		.depth = 32,
		.class = TrueColor
	};

	long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;

	infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
	visual = NULL;

	for (i = 0; i < nitems; i++){
		fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = infos[i].visual;
			depth = infos[i].depth;
			cmap = XCreateColormap(dpy, root, visual, AllocNone);
			usergb = 1;
			break;
		}
	}

	XFree(infos);

	if (!visual) {
		visual = DefaultVisual(dpy, screen);
		depth = DefaultDepth(dpy, screen);
		cmap = DefaultColormap(dpy, screen);
	}
}


void initialize() {
	XWindowAttributes wa;

  if (!(dpy = XOpenDisplay(NULL))) {
    exit(1);
  }

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	if (!XGetWindowAttributes(dpy, root, &wa)) {
		exit(1);
  }

  xinitvisual();
	drw = drw_create(dpy, screen, root, wa.width, wa.height, visual, depth, cmap);

	/*if (!drw_fontset_create(drw, (const char**) fonts, LENGTH(fonts))) {*/
    /*exit(1);*/
  /*}*/

  XSetWindowAttributes swa;
  XClassHint ch = {"popcorn", "popcorn"};

	/* create menu window */
	swa.override_redirect = True;
	swa.background_pixel = CWBackPixel;
	swa.border_pixel = 0;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | VisibilityChangeMask;
  win = XCreateWindow(dpy, root, x, y, mw, mh, border_width,
                    depth, InputOutput, visual,
                    CWOverrideRedirect | CWBackPixel | CWColormap |  CWEventMask | CWBorderPixel, &swa);
	XSetWindowBorder(dpy, win, CWBackPixel);
	XSetClassHint(dpy, win, &ch);
  XMapRaised(dpy, win);

  drw_resize(drw, mw, mh);
}

int main() {
  XSetErrorHandler(error_handler);

  int running = 1;

  Display *dpy = XOpenDisplay(0);
  Window root = DefaultRootWindow(dpy);

  // Grab keys
  // for (i = 0; i < LENGTH(keys); i++) {
  //   bind_key(dpy, root, keys[i].mod, keys[i].key);
  // }

  /*XSelectInput(dpy, root, KeyPressMask);*/

  /*readstdin();*/
  /* this variable is our reference to the second thread */
  pthread_t inc_x_thread;

  /* create a second thread which executes inc_x(&x) */
  if(pthread_create(&inc_x_thread, NULL, readstdin, &x)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
 
  initialize();

  /* main event loop */
  XEvent ev;
  while (running && !XNextEvent(dpy, &ev)) {
    switch (ev.type) {
      case Expose:
        if (ev.xexpose.count == 0)
          drw_map(drw, win, 0, 0, mw, mh);
        break;
      case VisibilityNotify:
        if (ev.xvisibility.state != VisibilityUnobscured)
          XRaiseWindow(dpy, win);
        break;
      case KeyPress: {
        // keypress(dpy, root, &ev.xkey);
        break;
      }
    }
  }

  XCloseDisplay(dpy);
}

