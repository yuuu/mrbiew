cmake_minimum_required(VERSION 3.16)
project(mrbiew C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

option(MRBIEW_SANITIZE "Enable ASan/UBSan" OFF)

set(TP ${CMAKE_SOURCE_DIR}/third_party)
foreach(dep webview mruby cJSON)
  if(NOT EXISTS ${TP}/${dep})
    message(FATAL_ERROR "third_party/${dep} not found. Run tools/fetch_third_party.sh")
  endif()
endforeach()

if(MRBIEW_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# ---- mruby (rake でビルドし静的ライブラリを取り込む) ----
set(MRUBY_BUILD_DIR ${CMAKE_BINARY_DIR}/mruby)
set(MRUBY_LIB ${MRUBY_BUILD_DIR}/lib/libmruby.a)
add_custom_command(
  OUTPUT ${MRUBY_LIB}
  COMMAND ${CMAKE_COMMAND} -E env
          MRUBY_CONFIG=${CMAKE_SOURCE_DIR}/build_config/mruby.rb
          MRUBY_BUILD_DIR=${MRUBY_BUILD_DIR}
          rake -j4
  WORKING_DIRECTORY ${TP}/mruby
  COMMENT "Building mruby"
  VERBATIM)
add_custom_target(mruby_build DEPENDS ${MRUBY_LIB})
add_library(mruby STATIC IMPORTED GLOBAL)
set_target_properties(mruby PROPERTIES
  IMPORTED_LOCATION ${MRUBY_LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${TP}/mruby/include)
add_dependencies(mruby mruby_build)

# ---- cJSON ----
add_library(cjson STATIC ${TP}/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC ${TP}/cJSON)

# ---- webview (C++ 実装を1TUだけ静的ライブラリ化。利用側はC API) ----
add_library(webview_impl STATIC ${TP}/webview/core/src/webview.cc)
target_include_directories(webview_impl PUBLIC ${TP}/webview/core/include)
target_compile_definitions(webview_impl PUBLIC WEBVIEW_STATIC)
if(APPLE)
  target_link_libraries(webview_impl PUBLIC "-framework WebKit" "-framework Cocoa")
elseif(UNIX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
  pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
  target_include_directories(webview_impl PUBLIC ${GTK3_INCLUDE_DIRS})
  target_link_libraries(webview_impl PUBLIC PkgConfig::WEBKIT ${GTK3_LIBRARIES} dl)
endif()

# ---- runtime / core ----
add_library(mrbiew_runtime STATIC
  runtime/runtime.c
  runtime/json_bridge.c)
target_include_directories(mrbiew_runtime PUBLIC runtime core)
target_link_libraries(mrbiew_runtime PUBLIC mruby cjson m)
target_compile_options(mrbiew_runtime PRIVATE -Wall -Wextra -Werror)

add_executable(mrbiew runtime/main.c)
target_link_libraries(mrbiew PRIVATE mrbiew_runtime webview_impl)
target_compile_options(mrbiew PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(mrbiew PRIVATE MRBIEW_APP_DIR="${CMAKE_SOURCE_DIR}/app")
