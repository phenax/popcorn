#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "config.h"

#define LENGTH(X) (sizeof X / sizeof X[0])
#define CLEANMASK(mask) (mask & ~LockMask & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

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

int main() {
  XSetErrorHandler(error_handler);

  int running = 1, i = 0;

  Display *dpy = XOpenDisplay(0);
  Window root = DefaultRootWindow(dpy);

  // Grab keys
  // for (i = 0; i < LENGTH(keys); i++) {
  //   bind_key(dpy, root, keys[i].mod, keys[i].key);
  // }

  XSelectInput(dpy, root, KeyPressMask);

  /* main event loop */
  XEvent ev;
  XSync(dpy, False);
  while (running) {
    XMaskEvent(dpy, KeyPressMask, &ev);

    switch (ev.type) {
      case KeyPress: {
        // keypress(dpy, root, &ev.xkey);
        break;
      }
    }
  }

  XCloseDisplay(dpy);
}

