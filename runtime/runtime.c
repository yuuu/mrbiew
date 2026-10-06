#include "runtime.h"

#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_bridge.h"

#define MAX_SCRIPT_SIZE (4 * 1024 * 1024)

struct Runtime {
  mrb_state *mrb;
};

/* Phase 4 でバイトコード埋め込みに置き換える予定のプレリュード */
static const char PRELUDE[] =
  "module App\n"
  "  @handlers = {}\n"
  "  def self.on(name, &blk)\n"
  "    @handlers[name.to_s] = blk\n"
  "  end\n"
  "  def self.dispatch(name, args)\n"
  "    h = @handlers[name]\n"
  "    return { error: \"unknown handler: #{name}\" } unless h\n"
  "    h.call(args)\n"
  "  rescue => e\n"
  "    { error: e.message }\n"
  "  end\n"
  "end\n";

static char *read_file(const char *path, size_t *len)
{
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;
  if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return NULL; }
  long size = ftell(fp);
  if (size < 0 || size > MAX_SCRIPT_SIZE || fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    return NULL;
  }
  char *buf = malloc((size_t)size + 1);
  if (!buf) { fclose(fp); return NULL; }
  size_t n = fread(buf, 1, (size_t)size, fp);
  fclose(fp);
  if (n != (size_t)size) { free(buf); return NULL; }
  buf[n] = '\0';
  *len = n;
  return buf;
}

static int check_exc(mrb_state *mrb, const char *what)
{
  if (!mrb->exc) return 0;
  mrb_value s = mrb_inspect(mrb, mrb_obj_value(mrb->exc));
  fprintf(stderr, "mruby error (%s): %s\n", what, mrb_str_to_cstr(mrb, s));
  mrb->exc = NULL;
  return -1;
}

static int load_source(mrb_state *mrb, const char *src, size_t len, const char *what)
{
  mrb_load_nstring(mrb, src, len);
  return check_exc(mrb, what);
}

Runtime *runtime_new(const char *main_rb_path)
{
  size_t len = 0;
  char *src = read_file(main_rb_path, &len);
  if (!src) {
    fprintf(stderr, "cannot read %s\n", main_rb_path);
    return NULL;
  }
  Runtime *rt = calloc(1, sizeof(*rt));
  if (!rt) { free(src); return NULL; }
  rt->mrb = mrb_open();
  if (!rt->mrb ||
      load_source(rt->mrb, PRELUDE, strlen(PRELUDE), "prelude") != 0 ||
      load_source(rt->mrb, src, len, main_rb_path) != 0) {
    free(src);
    runtime_free(rt);
    return NULL;
  }
  free(src);
  return rt;
}

void runtime_free(Runtime *rt)
{
  if (!rt) return;
  if (rt->mrb) mrb_close(rt->mrb);
  free(rt);
}

static char *error_json(const char *msg)
{
  cJSON *o = cJSON_CreateObject();
  if (!o) return NULL;
  cJSON_AddStringToObject(o, "error", msg);
  char *out = cJSON_PrintUnformatted(o);
  cJSON_Delete(o);
  return out;
}

char *runtime_dispatch(Runtime *rt, const char *name, const cJSON *args)
{
  mrb_state *mrb = rt->mrb;
  int ai = mrb_gc_arena_save(mrb);
  char *out = NULL;

  mrb_value app = mrb_const_get(mrb, mrb_obj_value(mrb->object_class), mrb_intern_lit(mrb, "App"));
  mrb_value argv[2];
  argv[0] = mrb_str_new_cstr(mrb, name);
  argv[1] = json_to_mrb(mrb, args);
  mrb_value ret = mrb_funcall_argv(mrb, app, mrb_intern_lit(mrb, "dispatch"), 2, argv);

  if (mrb->exc) {
    mrb_value s = mrb_obj_as_string(mrb, mrb_obj_value(mrb->exc));
    out = error_json(mrb_str_to_cstr(mrb, s));
    mrb->exc = NULL;
  } else {
    cJSON *j = mrb_to_json(mrb, ret);
    if (j) {
      out = cJSON_PrintUnformatted(j);
      cJSON_Delete(j);
    } else {
      out = error_json("cannot serialize handler result");
    }
  }
  mrb_gc_arena_restore(mrb, ai);
  return out;
}
