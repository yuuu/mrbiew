#!/bin/sh
# third_party の依存をバージョン固定で取得する。
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DEST="$ROOT/third_party"
mkdir -p "$DEST"

fetch() {
  name=$1; url=$2; tag=$3
  if [ -d "$DEST/$name/.git" ]; then
    echo "skip: $name (already present)"
    return
  fi
  echo "fetch: $name $tag"
  git clone --quiet -c advice.detachedHead=false --depth 1 --branch "$tag" "$url" "$DEST/$name"
}

fetch webview https://github.com/webview/webview.git 0.12.0
fetch mruby   https://github.com/mruby/mruby.git     3.4.0
fetch cJSON   https://github.com/DaveGamble/cJSON.git v1.7.18
