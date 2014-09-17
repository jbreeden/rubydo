# Global Build tool configuration
# ======================

$CPP = ENV['CPP'] || "g++"
$CC = ENV['CC'] || "gcc"
$RUBY = ENV['RUBY'] || "C:/projects/lib/rubyinstaller-master/sandbox/ruby20_mingw"
$RUBYDLL = ENV['RUBYDLL'] || "x64-msvcrt-ruby200.dll"

# Global Compilation Options
# ====================

$COMPILE_FLAGS = [
  "-std=c++11",
  "-fpermissive"
]

$INCLUDE_PATHS = [
  "include",
  "#{$RUBY}/include/ruby-2.0.0",
  "#{$RUBY}/include/ruby-2.0.0/x64-mingw32",
]

def compile_options
  flags = $COMPILE_FLAGS.join(" ")
  search_paths = $INCLUDE_PATHS.map { |path| "-I#{path}" }.join(" ")
  "#{flags} #{search_paths}"
end

# Global Linking Options
# ================

$ARTIFACT = "librubydo.a"

$LINK_FLAGS = []

$LIBRARY_PATHS = [
  "#{$RUBY}/lib"
]

$LIBRARIES = []

def link_options
  flags = $LINK_FLAGS.join(" ")
  search_paths = $LIBRARY_PATHS.map { |path| "-L#{path}" }.join(" ")
  "#{flags} #{search_paths}"
end

def link_libraries
  $LIBRARIES.map { |lib| "-l#{lib}" }.join(" ")
end

# Global File Lists
# ===========
  
$SOURCE_FILES = FileList["./**/*.{c,cpp}"]

# Debug Configuration Tasks
# ===================

namespace :debug do
  # Customize compile options for this configuration
  task :compile_options do
    $COMPILE_FLAGS += %w[-DDEBUG]
    $INCLUDE_PATHS += []
  end
  
  # Customize link options for this configuration
  task :link_options do
    $LINK_FLAGS += []
    $LIBRARY_PATHS += ["#{$RUBY}/bin"]
    $LIBRARIES += [$RUBYDLL]
  end
  
  directory "Debug"
  
  # Derive the object files & output directories from the source files
  obj_files = $SOURCE_FILES.pathmap("Debug/obj/%X.o")
  output_directories = obj_files.map { |obj| File.dirname obj }
    
  # Make a directory task for each output folder
  output_directories.each { |dir| directory dir }
  
  # Make a file task for each object file
  $SOURCE_FILES.zip(obj_files).each do |source, obj|
    file obj => source do
      result = (sh "#{$CPP} #{compile_options} -c #{source} -o #{obj}")
      puts "[RAKE] Compilation #{result ? "Succeeded" : "Failed"}"
    end
  end
  
  file "Debug/test.rb" => "test.rb" do
    cp "test.rb", "Debug/test.rb"
  end
  
  desc "Compile all sources"
  task :compile => ([:compile_options] + output_directories + obj_files)
  
  task :link => %w[link_options compile] do
  desc "Link the Debug artifact"
    result = sh "g++ -o Debug/rubydo_test.exe  #{link_options} #{obj_files.join(' ')} #{link_libraries}"
    puts "[RAKE] Linking #{result ? "Succeeded" : "Failed"}"
  end
  
  desc "Build the Debug configuration"
  task :build => ["Debug", "compile", "link", "Debug/test.rb"]
end

# Release Configuration Tasks
# =====================

namespace :release do
  # Customize compile options for this configuration
  task :compile_options do
    $COMPILE_FLAGS += %w[-DRELEASE]
    $INCLUDE_PATHS += []
  end
  
  # Customize link options for this configuration
  task :link_options do
    $LINK_FLAGS += []
    $LIBRARY_PATHS += []
    $LIBRARIES += []
  end
  
  directory "Release"
  
  # Derive the object files & output directories from the source files
  obj_files = $SOURCE_FILES.pathmap("Release/obj/%X.o")
  output_directories = obj_files.map { |obj| File.dirname obj }
    
  # Make a directory task for each output folder
  output_directories.each { |dir| directory dir }
  
  # Make a file task for each object file
  $SOURCE_FILES.zip(obj_files).each do |source, obj|
    file obj => source do
      result = (sh "#{$CPP} #{compile_options} -c #{source} -o #{obj}")
      puts "[RAKE] Compilation #{result ? "Succeeded" : "Failed"}"
    end
  end
  
  desc "Compile all sources"
  task :compile => ([:compile_options] + output_directories + obj_files)
  
  task :link => %w[link_options compile] do
  desc "Link the Release artifact"
    result = sh "ar -r Release/#{$ARTIFACT} #{obj_files.join(' ')}"
    puts "[RAKE] Linking #{result ? "Succeeded" : "Failed"}"
  end
  
  desc "Build the Release configuration"
  task :build => ["Release", "compile", "link"]
end

# Clean task
# ========

task :clean do
  rm_rf "Debug" if File.exists? "Debug"
  rm_rf "Release" if File.exists? "Release"
  rm_rf "Dist" if File.exists? "Dist"
end
