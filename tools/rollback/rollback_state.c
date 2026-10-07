/*
 * EmulatorJS netplay rollback - state serialization.
 *
 * APPEND THIS FILE to RetroArch/tasks/task_save.c (the file that defines
 * save_state_info()/load_state()). It reuses the in-memory helpers already
 * defined in that translation unit:
 *
 *   size_t content_get_serialized_size_rewind(void);
 *   bool   content_serialize_state_rewind(void *buffer, size_t buffer_size);
 *   bool   content_deserialize_state(const void *s, size_t len);
 *
 * Exports (add to Makefile.emulatorjs EXPORTED_FUNCTIONS):
 *   _ejs_state_size _ejs_save_state _ejs_load_state
 */

#include <emscripten/emscripten.h>

EMSCRIPTEN_KEEPALIVE
int ejs_state_size(void)
{
   return (int)content_get_serialized_size_rewind();
}

EMSCRIPTEN_KEEPALIVE
int ejs_save_state(unsigned char *dst, int cap)
{
   size_t n = content_get_serialized_size_rewind();
   if (!dst || cap <= 0 || n == 0 || (size_t)cap < n)
      return -1;
   if (!content_serialize_state_rewind(dst, (size_t)cap))
      return -1;
   return (int)n;
}

EMSCRIPTEN_KEEPALIVE
int ejs_load_state(const unsigned char *src, int len)
{
   if (!src || len <= 0)
      return 0;
   return content_deserialize_state(src, (size_t)len) ? 1 : 0;
}

/* Full (non-rewind) format, for cross-instance portability testing. */

EMSCRIPTEN_KEEPALIVE
int ejs_state_size_full(void)
{
   return (int)content_get_serialized_size();
}

EMSCRIPTEN_KEEPALIVE
int ejs_save_state_full(unsigned char *dst, int cap)
{
   size_t len = 0;
   void *data = content_get_serialized_data(&len);
   if (!data)
      return -1;
   if (!dst || cap <= 0 || (size_t)cap < len)
   {
      free(data);
      return -1;
   }
   memcpy(dst, data, len);
   free(data);
   return (int)len;
}
