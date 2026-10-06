#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webview/webview.h>

#include "runtime.h"

#define MAX_REQUEST_SIZE (1024 * 1024)

typedef struct {
  webview_t w;
  Runtime *rt;
} App;

static void return_error(webview_t w, const char *id, const char *msg)
{
  cJSON *o = cJSON_CreateObject();
  if (!o) return;
  cJSON_AddStringToObject(o, "error", msg);
  char *s = cJSON_PrintUnformatted(o);
  cJSON_Delete(o);
  if (s) {
    webview_return(w, id, 0, s);
    free(s);
  }
}

/* window.invoke(name, args) -> req = '["name", {...}]' */
static void on_invoke(const char *id, const char *req, void *arg)
{
  App *app = arg;
  if (strlen(req) > MAX_REQUEST_SIZE) {
    return_error(app->w, id, "request too large");
    return;
  }
  cJSON *root = cJSON_Parse(req);
  const cJSON *name = cJSON_IsArray(root) ? cJSON_GetArrayItem(root, 0) : NULL;
  if (!cJSON_IsString(name) || !name->valuestring) {
    return_error(app->w, id, "invalid request");
    cJSON_Delete(root);
    return;
  }
  const cJSON *args = cJSON_GetArrayItem(root, 1); /* 無ければ NULL -> nil */
  if (args && !cJSON_IsObject(args) && !cJSON_IsNull(args)) {
    return_error(app->w, id, "args must be an object");
    cJSON_Delete(root);
    return;
  }
  char *res = runtime_dispatch(app->rt, name->valuestring, args);
  if (res) {
    webview_return(app->w, id, 0, res);
    free(res);
  } else {
    return_error(app->w, id, "internal error");
  }
  cJSON_Delete(root);
}

int main(void)
{
  const char *dir = getenv("MRBIEW_APP_DIR"); /* devモード: 実行時に上書き可 */
  if (!dir) dir = MRBIEW_APP_DIR;

  char main_rb[1024], url[1100];
  snprintf(main_rb, sizeof(main_rb), "%s/main.rb", dir);
  snprintf(url, sizeof(url), "file://%s/ui/index.html", dir);

  App app = {0};
  app.rt = runtime_new(main_rb);
  if (!app.rt) return 1;

  app.w = webview_create(0, NULL);
  if (!app.w) {
    runtime_free(app.rt);
    return 1;
  }
  webview_set_title(app.w, "mrbiew");
  webview_set_size(app.w, 480, 320, WEBVIEW_HINT_NONE);
  webview_bind(app.w, "invoke", on_invoke, &app);
  webview_navigate(app.w, url);
  webview_run(app.w);

  webview_destroy(app.w);
  runtime_free(app.rt);
  return 0;
}
