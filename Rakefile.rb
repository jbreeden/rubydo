require './rakelib/rake_gcc.rb'

include RakeGcc

# Global Build tool configuration
# ======================

$CPP = ENV['CPP'] || "g++"
$CC = ENV['CC'] || "gcc"
$RUBY = ENV['RUBY'] || "C:/projects/lib/rubyinstaller-master/sandbox/ruby20_mingw"
$RUBYDLL = ENV['RUBYDLL'] || "x64-msvcrt-ruby200.dll"

# Debug Configuration
# ===================

build_target :debug do
  compiler 'g++'

  file "#{@build_target.name}/test.rb" => 'test.rb' do
    cp 'test.rb', "#{@build_target.name}/test.rb"
  end

  file "#{@build_target.name}/#{$RUBYDLL}" => "#{$RUBY}/bin/#{$RUBYDLL}" do
    cp "#{$RUBY}/bin/#{$RUBYDLL}", "#{@build_target.name}/#{$RUBYDLL}"
  end

  compile do
    depend "#{@build_target.name}/test.rb"
    depend "#{@build_target.name}/#{$RUBYDLL}"
    flags "-std=c++11", "-fpermissive"
    define 'DEBUG'
    search [
      "include",
      "#{$RUBY}/include/ruby-2.0.0",
      "#{$RUBY}/include/ruby-2.0.0/x64-mingw32",
    ]
    sources ["src/rubydo.cpp", "src/ruby_class.cpp", "src/ruby_module.cpp"]
  end

  link do
    search "#{$RUBY}/lib"
    lib $RUBYDLL
    artifact 'rubydo.exe'
  end
end

# Release Configuration
# =====================

build_target :release, :debug do
  compile do
    clear_dependencies
    undefine 'DEBUG'
    define 'RELEASE'
  end

  link do
    artifact 'librubydo.a'
  end
end

# Clean Task
# ==========

desc "Clean all build targets"
task :clean do
  rm_rf 'debug' if File.exists? 'debug'
  rm_rf 'release' if File.exists? 'release'
end
