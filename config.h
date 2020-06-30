#include <X11/XF86keysym.h>

// Shell
char shell[] = "/bin/sh";

// Define normal mode key bindings here
Key keys[] = {
  { Mod1Mask,  XK_y,             cmd("notify-send hello") },
  { Mod1Mask,  XK_z,             mode(Music, True) },
  { Mod1Mask,  XF86XK_PowerOff,  mode(Power, False) },
};

