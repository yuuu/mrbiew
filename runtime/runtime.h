#ifndef MRBIEW_RUNTIME_H
#define MRBIEW_RUNTIME_H

#include <cJSON.h>

typedef struct Runtime Runtime;

/* main.rb を読み込んで初期化する。失敗時は NULL。mrb_state はメインスレッド専用。 */
Runtime *runtime_new(const char *main_rb_path);
void runtime_free(Runtime *rt);

/* ハンドラ呼び出し。name は許可リスト(App.on 登録済み)のみ通る。
 * 戻り値は JSON 文字列(呼び出し側が free)。エラーは {"error": "..."}。 */
char *runtime_dispatch(Runtime *rt, const char *name, const cJSON *args);

#endif
