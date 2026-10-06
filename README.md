# mrbiew

> **⚠️ Work in progress.** This project is in early development (Phase 1: minimal prototype).
> APIs, the IPC protocol, and the directory layout will change without notice. Not ready for production use.

A tiny framework for building desktop apps with **C11 + [mruby](https://mruby.org/) + a system WebView** — no Rust, no Node, no Electron.

```
[HTML/CSS/JS UI]  ⇄  JSON IPC  ⇄  [mruby: routing / app logic]  ⇄  [C: core / native APIs]
        └──────────── webview/webview (C API) ────────────┘
```

It is not a Tauri replacement. The goal is to show that C and Ruby alone are enough to build a fast desktop app with a minimal footprint.

## Status

| Phase | Description | State |
|---|---|---|
| 1 | Window + WebView, mruby, `invoke("greet")` round trip | ✅ Working on macOS |
| 2 | `Native.*` C functions callable from Ruby, JSON ⇄ mruby conversion tests | Planned |
| 3 | Worker threads for heavy C work, non-blocking UI | Planned |
| 4 | Single binary (embedded bytecode and assets) | Planned |
| 5 | Packaging and a small CLI (`new` / `dev` / `build`) | Planned |

## Quick start (macOS)

Requirements: Xcode Command Line Tools, CMake, Ruby (used by mruby's `rake` build), git.

```sh
tools/fetch_third_party.sh   # fetches webview, mruby and cJSON into third_party/
cmake -S . -B build
cmake --build build
./build/mrbiew
```

Click **Greet**: the JS calls `invoke("greet")`, which is routed to the Ruby handler in `app/main.rb`, and the result is shown in the page.

Linux requires `libgtk-3-dev` and `libwebkit2gtk-4.1-dev`; this path is untested so far.
Windows is not yet supported.

## How it works

### Ruby handlers

```ruby
# app/main.rb
App.on("greet") { |args| { message: "Hello, #{args["name"]}" } }
```

### Calling from JavaScript

```js
const res = await window.invoke("greet", { name: "mruby" });
console.log(res.message);
```

- Only handlers registered with `App.on` can be called (allow-list). Unknown names return `{ "error": "..." }`.
- Arguments must be a JSON object. Hashes, arrays, strings, integers, floats, `nil` and booleans round-trip between JSON and mruby.

## Design rules

- mruby is not thread-safe: the `mrb_state` is only touched from the main thread.
- All external input (IPC JSON, argument types, sizes) is validated.
- The WebView loads local assets only, guarded by a Content Security Policy.
- Dependencies are kept minimal: webview, mruby, cJSON.

## Development

- Set `MRBIEW_APP_DIR` to load `main.rb` and `ui/` from another directory (dev mode).
- Sanitizer build: `cmake -S . -B build-asan -DMRBIEW_SANITIZE=ON`
- Compiled with `-Wall -Wextra -Werror`.

## Layout

```
core/       native C logic (planned)
runtime/    mruby setup, JSON bridge, IPC glue
app/        main.rb and ui/ (HTML/CSS/JS)
tools/      helper scripts
build_config/  mruby build configuration
```

## License

[MIT](LICENSE)
