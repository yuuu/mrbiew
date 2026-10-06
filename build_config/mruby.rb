# mruby ビルド設定。出力先は CMake から MRUBY_BUILD_DIR で渡される。
MRuby::Build.new do |conf|
  conf.toolchain :gcc
  conf.build_dir = ENV.fetch("MRUBY_BUILD_DIR")
  conf.gembox "default"
  conf.cc.flags << "-fPIC" << "-O2"
end
