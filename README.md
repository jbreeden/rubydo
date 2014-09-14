Rubydo
======

Convenience functions and syntax sugar for using C++ lambdas with the Ruby C API. The library is fairly small, currently consisting of a few functions for initializing ruby, obtaining/releasing the giant VM lock (GVL - or sometimes GIL for "global interpreter lock"), launching ruby threads, and defining ruby classes & methods. The use of C++ lambdas for these purposes allows for cleaner and more dynamic code than the bare C API provided by ruby.

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
ruby::init(argc, argv);
```

Using the GVL
-------------
  
```C++
VALUE result;
ruby::without_gvl( DO [&](){
  /* GVL is released, this code will execute in parallel to any other ruby threads */
  
  /* Doing some heavy computation... */
  
  /* Need to call a ruby method, grab the GVL */
  ruby::with_gvl DO [&]() {
    rb_funcall(rb_mKernel, rb_intern("puts"), 1, some_rb_string);
  } END;
  
  result = some_rb_string;
} END, DO [](){ /* unblock */ } END);

/* Now that we have the GVL again, it's safe to call ruby methods */
rb_funcall(rb_mKernel, rb_intern("puts"), 1, result);
```

Launching a Ruby Thread
-----------------------

```C++
VALUE create_thread () {
  cout << "In create_thread" << endl;
  VALUE message = rb_str_new_cstr("In the ruby thread");

  /* The DO block creates a shared_ptr to a new std::function,
     so even if this method returns before the new thread is scheduled,
     the lambda will still exist. Once the thread exists, it releases
     it's copy of the shared_ptr */
  VALUE thread = ruby::thread DO [&, message] () {
   rb_funcall(rb_mKernel, rb_intern("puts"), 1, message);
  } END;
  
  return thread;
}

int main (int argc, char** argv) {
  ruby::init(argc, argv);
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

Creating Ruby Classes
---------------------

Rubydo allows you to dynamically create Ruby classes with C++ lambdas. Here are a few examples from rubydo.cpp's self-testing main method:

```C++
// Defining a top-level class with a method
auto ruby_do_test_class = rubydo::RubyClass("RubydoTest");
ruby_do_test_class.define_method("test_method_returns_success", [](VALUE self, int argc, VALUE* argv){
  return rb_str_new_cstr("success");
});

// Defining a class under another class, with a method of it's own
auto nested_class = ruby_do_test_class.define_class("NestedClass");
nested_class.define_method("nested_class_method", [](VALUE self, int argc, VALUE* argv){
  return rb_str_new_cstr("success");
});

// Opening an existing ruby class from the VALUE object of the class and monkey patching it with a new method
auto object_class = rubydo::RubyClass(rb_cObject);
object_class.define_method("rubydo_monkey_patch_by_value", [](VALUE self, int argc, VALUE* argv){
  return rb_str_new_cstr("success");
});

// Opening an existing class by name and monkey patching it
auto object_class_2 = rubydo::RubyClass("Object");
object_class_2.define_method("rubydo_monkey_patch_by_name", [](VALUE self, int argc, VALUE* argv){
  return rb_str_new_cstr("success");
});
```

You can see the usage of the defined classes and methods from the ruby side in test.rb
