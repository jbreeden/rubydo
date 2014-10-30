Rubydo
======

Convenience functions and syntax sugar for using C++ lambdas with the Ruby C API. The library is fairly small, currently  supporting initializing ruby, defining modules and classes, obtaining/releasing the giant VM lock (GVL - or sometimes GIL for "global interpreter lock"), and launching ruby threads. The use of C++ lambdas for these purposes allows for cleaner and more dynamic code than the bare C API provided by ruby.

Goals
-----

Rubydo aims to be

- __Simple__ Rubydo code should be easily understood by anybody with moderate comfort in C++ (after a quick look through [Extending Ruby](http://media.pragprog.com/titles/ruby3/ext_ruby.pdf) anyway).
- __Familiar__ If you're familiar with the Ruby C API, the conventions used in Rubydo should feel natural aside from occasionally being more object-oriented.
- __Productive__ The functionality present in Rubydo should make it much simpler to interface with Ruby from C++ code.
- __Powerful__ Rubydo shold make writing extensions feel more like writing Ruby. Much of the power and dynamics of Ruby comes from blocks and closures. By using lambdas many of these same qualities can be enjoyed by native extensions.
- __Complementary__ Rubydo should not re-implement things that the Ruby C API already does well for the sole purpose of object-orientation. Instead, Rubydo should remain small, providing a thin adapter over the underlying API to facilitate the use of C++ features where appropriate. This will ease the task of adapting to future Ruby releases.

Usage
=====

Initializing Ruby
-----------------

```C++
/* initializing ruby */
rubydo::init(argc, argv);
```

Using the Ruby Standard Library
-------------------------------

If you're embedding Ruby in a C++ application, instead of writing an extension, and you plan to include & use the Ruby standard library or any gems, there is a extra step in the initialization process:

```C++
/* initializing ruby */
rubydo::init(argc, argv);

/* initializing some things to make requires of libraries and gems work correctly */
rubydo::use_ruby_standard_library();
```

To set this up, you just copy the `lib` folder from your build of Ruby into the same folder of your application's exe.

Ex:

```
ApplicationFolder/
 - app.exe
 - lib/
   - ruby/
     - {VERSION_NUMBER}/
     - gems/
     - site_ruby/
     - vendor_ruby/
```

Creating Ruby Modules and Classes
---------------------------------

Rubydo allows you to dynamically create Ruby modules and classes, defining their methods with C++ lambdas. All lambdas defining methods in rubydo use the argc/argv calling convention `[](VALUE self, int argc, VALUE* argv){...}`. There is not yet any real documentation, but here are the examples from rubydo.cpp's self-testing main method:

```C++
// Defining a module
RubyModule rubydo_module = RubyModule::define("RubydoModule");

// Defining a class
RubyClass rubydo_class = RubyClass::define("RubydoClass");

// Defining a singleton method on a module
rubydo_module
  .define_singleton_method("module_singleton_method", [](VALUE self, int argc, VALUE* argv){
    return rb_str_new_cstr("success");
  });

// Defining a singleton method on a class
rubydo_class
  .define_singleton_method("class_singleton_method", [](VALUE self, int argc, VALUE* argv){
    return rb_str_new_cstr("success");
  });

// Defining an instance method on a class
rubydo_class.define_method("class_instance_method", [](VALUE self, int argc, VALUE* argv){
  return rb_str_new_cstr("success");
});

// Defining an instance method on a module
rubydo_module.define_method("module_instance_method", [](VALUE self, int argc, VALUE* argv){
  return rb_str_new_cstr("success");
});

// Defining a class under another class
rubydo_class.define_class("NestedClass")
  .define_method("nested_class_method", [](VALUE self, int argc, VALUE* argv){
    return rb_str_new_cstr("success");
  });

// Opening an existing ruby class from the VALUE object of the class and monkey patching it with a new method
RubyClass::define(rb_cObject)
  .define_method("rubydo_monkey_patch_by_value", [](VALUE self, int argc, VALUE* argv){
    return rb_str_new_cstr("success");
  });

// Opening an existing class by name and monkey patching it
RubyClass::define("Object")
  .define_method("rubydo_monkey_patch_by_name", [](VALUE self, int argc, VALUE* argv){
    return rb_str_new_cstr("success");
  });

// Nesting a module in another module
rubydo_module.define_module("ModuleUnderModule");

// Nesting a class in a module
rubydo_module.define_class("ClassUnderModule");

// Nesting multiple levels of mixed modules & classes
RubyModule::define("Mod1").define_class("Class1").define_module("Mod2").define_class("Class2");

// Re-opening a nested class to define a method
RubyModule::define("Mod1").define_class("Class1").define_module("Mod2").define_class("Class2")
  .define_method("deeply_nested_method", [](VALUE self, int argc, VALUE* argv){
    return rb_str_new_cstr("success");
  });
```

You can see the usage of the defined classes and methods from the ruby side in test.rb

Using the GVL
-------------

```C++
VALUE result;
rubydo::without_gvl([&](){
  /* GVL is released, this code will execute in parallel to any other ruby threads */

  /* Doing some heavy computation... */

  /* Need to call a ruby method, grab the GVL */
  rubydo::with_gvl([&]() {
    rb_funcall(rb_mKernel, rb_intern("puts"), 1, some_rb_string);
  });

  result = some_rb_string;
}, [](){ /* unblock */ });

/* Now that we have the GVL again, it's safe to call ruby methods */
rb_funcall(rb_mKernel, rb_intern("puts"), 1, result);
```

Note: `rubydo::with_gvl` delegates to the `rb_thread_call_with_gvl` function, which will cause an error if called from a thread that already has the GVL. For this reason, rubydo tracks the GVL status in a thread local variable, allowing `rubydo::with_gvl` to execute the provided block directly if the thread already has the GVL, and delegating to `rb_thread_call_with_gvl` only if required. Mixing these calls with calls directly to the GVL functions provided by ruby is discouraged, and may cause errors.

Launching a Ruby Thread
-----------------------

```C++
VALUE create_thread () {
  cout << "In create_thread" << endl;
  VALUE message = rb_str_new_cstr("In the ruby thread");

  VALUE thread = rubydo::thread([&, message] () {
   rb_funcall(rb_mKernel, rb_intern("puts"), 1, message);
  });

  return thread;
}

int main (int argc, char** argv) {
  rubydo::init(argc, argv);
  auto thread = create_thread();
  cout << "Thread created" << endl;
  rb_funcall(thread, rb_intern("join"), 0);
  cout << "Thread joined" << endl;
}
```

OUTPUT:

```
In create_thread  
Thread created  
In the ruby thread  
Thread joined  
```
