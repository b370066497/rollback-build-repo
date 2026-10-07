/*
 * EmulatorJS netplay rollback - single-frame step + input latch.
 *
 * APPEND THIS FILE to RetroArch/retroarch.c (the file that defines
 * emscripten_mainloop() and the static emscripten_frame_count). Being in the
 * same translation unit lets us bump the frame counter and drive one frame.
 *
 * Determinism: we deliberately call core_run() (runloop.c) instead of
 * runloop_iterate(). core_run() runs exactly one core frame - input_poll +
 * retro_run - with no coupling to wall-clock time, audio-buffer status, the
 * menu/pause state machine, or runahead/preempt. runloop_iterate() feeds the
 * core a real-time delta through the frame-time callback and may run the
 * core extra times for runahead, which made frame stepping non-reproducible
 * (identical serialized state diverged within a few frames). core_run() is the
 * minimal deterministic step required for lockstep/rollback netplay.
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

/* defined in runloop.c - one core frame, no runloop timing/runahead coupling. */
extern void core_run(void);

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
   core_run();
}

EMSCRIPTEN_KEEPALIVE
int ejs_get_frame(void)
{
   return (int)emscripten_frame_count;
}
