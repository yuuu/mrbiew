#include "json_bridge.h"

#include <math.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/string.h>

#define MAX_DEPTH 64
#define MAX_SAFE_INT 9007199254740992.0 /* 2^53 */

static mrb_value from_json(mrb_state *mrb, const cJSON *j, int depth)
{
  if (depth > MAX_DEPTH) {
    return mrb_nil_value(); /* 深すぎる入力は nil に潰す(例外は投げない) */
  }
  if (j == NULL || cJSON_IsNull(j)) return mrb_nil_value();
  if (cJSON_IsTrue(j)) return mrb_true_value();
  if (cJSON_IsFalse(j)) return mrb_false_value();
  if (cJSON_IsNumber(j)) {
    double d = j->valuedouble;
    if (d == floor(d) && fabs(d) < MAX_SAFE_INT) return mrb_int_value(mrb, (mrb_int)d);
    return mrb_float_value(mrb, d);
  }
  if (cJSON_IsString(j)) {
    return mrb_str_new_cstr(mrb, j->valuestring ? j->valuestring : "");
  }
  int ai = mrb_gc_arena_save(mrb);
  if (cJSON_IsArray(j)) {
    mrb_value ary = mrb_ary_new(mrb);
    const cJSON *e;
    cJSON_ArrayForEach(e, j) {
      mrb_ary_push(mrb, ary, from_json(mrb, e, depth + 1));
      mrb_gc_arena_restore(mrb, ai);
      mrb_gc_protect(mrb, ary);
    }
    return ary;
  }
  if (cJSON_IsObject(j)) {
    mrb_value hash = mrb_hash_new(mrb);
    const cJSON *e;
    cJSON_ArrayForEach(e, j) {
      mrb_value k = mrb_str_new_cstr(mrb, e->string ? e->string : "");
      mrb_hash_set(mrb, hash, k, from_json(mrb, e, depth + 1));
      mrb_gc_arena_restore(mrb, ai);
      mrb_gc_protect(mrb, hash);
    }
    return hash;
  }
  return mrb_nil_value();
}

mrb_value json_to_mrb(mrb_state *mrb, const cJSON *json)
{
  return from_json(mrb, json, 0);
}

static cJSON *to_json(mrb_state *mrb, mrb_value v, int depth)
{
  if (depth > MAX_DEPTH) return NULL;
  switch (mrb_type(v)) {
  case MRB_TT_FALSE:
    return mrb_nil_p(v) ? cJSON_CreateNull() : cJSON_CreateFalse();
  case MRB_TT_TRUE:
    return cJSON_CreateTrue();
  case MRB_TT_INTEGER:
    return cJSON_CreateNumber((double)mrb_integer(v));
  case MRB_TT_FLOAT: {
    double d = mrb_float(v);
    return isfinite(d) ? cJSON_CreateNumber(d) : cJSON_CreateNull();
  }
  case MRB_TT_STRING:
    return cJSON_CreateString(mrb_str_to_cstr(mrb, v));
  case MRB_TT_ARRAY: {
    cJSON *arr = cJSON_CreateArray();
    if (!arr) return NULL;
    mrb_int n = RARRAY_LEN(v);
    for (mrb_int i = 0; i < n; i++) {
      cJSON *e = to_json(mrb, mrb_ary_ref(mrb, v, i), depth + 1);
      if (!e) { cJSON_Delete(arr); return NULL; }
      cJSON_AddItemToArray(arr, e);
    }
    return arr;
  }
  case MRB_TT_HASH: {
    cJSON *obj = cJSON_CreateObject();
    if (!obj) return NULL;
    mrb_value keys = mrb_hash_keys(mrb, v);
    mrb_int n = RARRAY_LEN(keys);
    for (mrb_int i = 0; i < n; i++) {
      mrb_value k = mrb_ary_ref(mrb, keys, i);
      mrb_value ks = mrb_obj_as_string(mrb, k);
      cJSON *e = to_json(mrb, mrb_hash_get(mrb, v, k), depth + 1);
      if (!e) { cJSON_Delete(obj); return NULL; }
      cJSON_AddItemToObject(obj, mrb_str_to_cstr(mrb, ks), e);
    }
    return obj;
  }
  default: {
    mrb_value s = mrb_obj_as_string(mrb, v);
    return cJSON_CreateString(mrb_str_to_cstr(mrb, s));
  }
  }
}

cJSON *mrb_to_json(mrb_state *mrb, mrb_value v)
{
  return to_json(mrb, v, 0);
}
