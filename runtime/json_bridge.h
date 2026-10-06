#ifndef MRBIEW_JSON_BRIDGE_H
#define MRBIEW_JSON_BRIDGE_H

#include <mruby.h>
#include <cJSON.h>

/* cJSON -> mrb_value。深さ上限超過は例外を送出する。 */
mrb_value json_to_mrb(mrb_state *mrb, const cJSON *json);

/* mrb_value -> cJSON。未対応型は to_s した文字列、NaN/Inf は null。
 * 戻り値は呼び出し側が cJSON_Delete する。失敗時は NULL。 */
cJSON *mrb_to_json(mrb_state *mrb, mrb_value v);

#endif
