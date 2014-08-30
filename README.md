rubydo
======

Convenience functions and syntax sugar for using C++ lambdas with the Ruby C API. The library is tiny, consisting of 
a few functions for initializing ruby, obtaining/releasing the giant VM lock (GVL - or sometimes GIL for "global interpreter lock"),
and launching ruby threads via lambdas.

Usage
=====

Initializing Ruby
-----------------

```
/* initializing ruby */
ruby::init(argc, argv);
```

Using the GVL
-------------
  
```
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

```
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

  cout << "Thread created" << endl;
  return thread;
}

int main (int argc, char** argv) {
  ruby::init(argc, argv);
  auto thread = create_thread();
  rb_funcall(thread, rb_intern("join"), 0);
  cout << "Thread joined" << endl;
}
```
