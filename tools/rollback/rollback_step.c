/*
 * EmulatorJS netplay rollback - single-frame step + input latch.
 *
 * APPEND THIS FILE to RetroArch/retroarch.c (the file that defines
 * emscripten_mainloop() and the static emscripten_frame_count). Being in the
 * same translation unit lets us bump the frame counter and drive one frame
 * exactly like the main loop does:
 *
 *   emscripten_frame_count++;
 *   runloop_iterate();
 *
 * Inputs are latched through simulate_input() (input/drivers/emulatorjs_input.c),
 * which only mutates the static keymap[] flags read by the input driver on the
 * next frame - so "set inputs, then step" is exactly one frame of input.
 *
 * Exports (add to Makefile.emulatorjs EXPORTED_FUNCTIONS):
 *   _ejs_set_frame_input _ejs_step_frame _ejs_get_frame
 */

#include <emscripten/emscripten.h>

/* defined in input/drivers/emulatorjs_input.c */
extern void simulate_input(int user, int key, int down);

EMSCRIPTEN_KEEPALIVE
void ejs_set_frame_input(int user, int key, int down)
{
   simulate_input(user, key, down);
}

/* Advance exactly one emulated frame with the currently latched inputs.
 * Callers must pause the normal main loop first (toggleMainLoop(0)). */
EMSCRIPTEN_KEEPALIVE
void ejs_step_frame(void)
{
   emscripten_frame_count++;
   runloop_iterate();
}

EMSCRIPTEN_KEEPALIVE
int ejs_get_frame(void)
{
   return (int)emscripten_frame_count;
}
