#include "rubydo.h"
#include "rubydo/ruby_class.h"
#include "ruby.h"
#include "ruby/thread.h"
#include <functional>

#ifdef DEBUG
#include <iostream>
#endif

// Helper functions
// ----------------
namespace {
  
  void invoke(void* arg) {
    if (arg != NULL) {
      DO_BLOCK *block = (DO_BLOCK*)arg;
      (**block)();
    }
  }

  void* invoke_returning_null_ptr(void* arg) {
    if (arg != NULL) {
      DO_BLOCK *block = (DO_BLOCK*)arg;
      (**block)();
    }
    return NULL;
  }

  VALUE invoke_and_destroy_returning_qnil (void* arg) {
    if (arg != NULL) {
      DO_BLOCK *block = (DO_BLOCK*)arg;
      (**block)();
      delete block;
    }
    return Qnil;
  }
}

namespace rubydo {

  // init
  // ----
  // Initializes the ruby vm, and requires a few ruby modules
  // needed to allow some of the standard libraries to load correctly.
  // ----
  void init(int argc, char** argv, bool using_std_lib) {
    ruby_sysinit(&argc, &argv);
    RUBY_INIT_STACK;
    ruby_init();
    ruby_init_loadpath();
    
    RubyModule::cRubydoMethod = rb_define_class("RubydoMethod", rb_cObject);
    
    if (using_std_lib) {
      rb_require("enc/encdb");
      rb_require("enc/trans/transdb");
    }
  }

  // without_gvl
  // -----------
  // Releases the GVL, executes the given DO_BLOCK `func`, and re-obtains the GVL.
  // If ruby needs to unblock `func` for any reason (such as an interrupt signal
  // has been given, or the thread is killed) then `ubf` will be called. `ubf`
  // is then responsible for unblocking `func` by some means.
  //
  // EXAMPLE:
  //
  //    /* Running some code with the GVL... */
  //    VALUE result;
  //    rubydo::without_gvl( DO [&]() {
  //      /* GVL is released, this code will execute in parallel to any ruby threads */
  //      result = some_rb_string;
  //    } END, DO [](){ /* unblock */ } END);
  //
  //    /* Now that we have the GVL again, it's safe to call ruby methods */
  //    rb_funcall(rb_mKernel, rb_intern("puts"), 1, result);
  // -----------
  void without_gvl(DO_BLOCK func, DO_BLOCK ubf) {
    rb_thread_call_without_gvl(invoke_returning_null_ptr, &func, invoke, &ubf);
  }

  // with_gvl
  // --------
  // Obtains the GVL, executes the given DO_BLOCK `func`, and releases the GVL
  //
  // EXAMPLE:
  //
  //    /* Running some code without the GVL... */
  //    VALUE message = some_rb_string;
  //    rubydo::with_gvl DO [&]() {
  //      /* Now that we have the GVL again, it's safe to call ruby methods */
  //      rb_funcall(rb_mKernel, rb_intern("puts"), 1, message);
  //    } END;
  // --------
  void with_gvl(DO_BLOCK func) {
    rb_thread_call_with_gvl(invoke_returning_null_ptr, &func);
  }

  // rb_do_thread
  // ------------
  // Spawns a ruby thread to execute the given function.
  // Returns the created ruby thread as a VALUE
  //
  // EXAMPLE:
  //
  //    VALUE create_thread () {
  //      cout << "In create_thread" << endl;
  //      VALUE message = rb_str_new_cstr("In the ruby thread");
  //
  //      VALUE thread = rubydo::thread DO [&, message] () {
  //       rb_funcall(rb_mKernel, rb_intern("puts"), 1, message);
  //      } END;
  //
  //      cout << "Thread created" << endl;
  //      return thread;
  //    }
  //
  //    int main (int argc, char** argv) {
  //      rubydo::init(argc, argv);
  //      auto thread = create_thread();
  //      rb_funcall(thread, rb_intern("join"), 0);
  //      cout << "Thread joined" << endl;
  //    }
  //
  // OUTPUT:
  //
  //    In create_thread
  //    Thread created
  //    In the ruby thread
  //    Thread joined
  //------------
  VALUE thread(DO_BLOCK thread_body) {
    // This is going on another thread, so copy the shared pointer
    // in case the client destroys their last reference before the
    // new thread is scheduled. This reference will be deleted by
    // invoke_and_destroy_do_block
    DO_BLOCK* ptr_to_copy = new DO_BLOCK(thread_body);
    return rb_thread_create(invoke_and_destroy_returning_qnil, ptr_to_copy);
  }
}

#ifdef DEBUG
 using namespace std;
 using namespace rubydo;
 
 int main(int argc, char** argv) {
  rubydo::init(argc, argv, false);
  
  // Defining a module
  RubyModule rubydo_module = RubyModule::define("RubydoModule");
  
  // Defining a class
  RubyClass rubydo_class = RubyClass::define("RubydoClass");
  
  // Defining a singleton method on a module
  RubyClass::define("RubydoModule")
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
  
  cout << "Loading test.rb" << endl;
  rb_require("./test.rb");
 }
#endif
