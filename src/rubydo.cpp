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

  thread_local bool thread_has_gvl = true;
  
  void invoke(void* arg) {
    if (arg != NULL) {
      (*((RUBYDO_BLOCK*)arg))();
    }
  }

  void* invoke_returning_null_ptr(void* arg) {
    if (arg != NULL) {
      (*((RUBYDO_BLOCK*)arg))();
    }
    return NULL;
  }

  VALUE invoke_and_destroy_returning_qnil (void* arg) {
    if (arg != NULL) {
      auto block_ptr = (RUBYDO_BLOCK*)arg;
      (*block_ptr)();
      delete block_ptr;
    }
    return Qnil;
  }
}

namespace rubydo {

  // init
  // ----
  // Initializes the ruby vm, and some internal rubydo state
  // ----
  void 
  init (int argc, char** argv) {
    ruby_sysinit(&argc, &argv);
    RUBY_INIT_STACK;
    ruby_init();
    ruby_init_loadpath();
    
    // Initialize the ruby class used to box Method objects in.
    RubyModule::cRubydoMethod = rb_define_class("RubydoMethod", rb_cObject);
  }
  
  // use_ruby_standard_library
  // -------------------------
  // If you're packaging the ruby standard libraries with your application,
  // and expecting to require libs and gems, this function will initializes
  // some things required to make sure your require statements work correctly.
  // -------------------------
  void 
  use_ruby_standard_library () {
    rb_require("enc/encdb");
    rb_require("enc/trans/transdb");
  }

  // without_gvl
  // -----------
  // Releases the GVL, executes the given RUBYDO_BLOCK `func`, and re-obtains the GVL.
  // If ruby needs to unblock `func` for any reason (such as an interrupt signal
  // has been given, or the thread is killed) then `ubf` will be called. `ubf`
  // is then responsible for unblocking `func` by some means.
  //
  // EXAMPLE:
  //
  //    /* Running some code with the GVL... */
  //    VALUE result;
  //    rubydo::without_gvl([&]() {
  //      /* GVL is released, this code will execute in parallel to any ruby threads */
  //      result = some_rb_string;
  //    }, [](){ /* unblock */ });
  //
  //    /* Now that we have the GVL again, it's safe to call ruby methods */
  //    rb_funcall(rb_mKernel, rb_intern("puts"), 1, result);
  // -----------
  void 
  without_gvl(RUBYDO_BLOCK func, RUBYDO_BLOCK ubf) {
    thread_has_gvl = false;
    rb_thread_call_without_gvl(invoke_returning_null_ptr, &func, invoke, &ubf);
  }

  // with_gvl
  // --------
  // Obtains the GVL, executes the given RUBYDO_BLOCK `func`, and releases the GVL.
  // Unlike the native rb_thread_call_with_gvl, this function is re-entrant, and
  // will only try to obtain the GVL if the current thread does not have it already.
  //
  // EXAMPLE:
  //
  //    /* Running some code without the GVL... */
  //    VALUE message = some_rb_string;
  //    rubydo::with_gvl([&]() {
  //      /* Now that we have the GVL again, it's safe to call ruby methods */
  //      rb_funcall(rb_mKernel, rb_intern("puts"), 1, message);
  //    });
  // --------
  void with_gvl(RUBYDO_BLOCK func) {
    if (!thread_has_gvl){
      thread_has_gvl = true;
      rb_thread_call_with_gvl(invoke_returning_null_ptr, &func);
    } else {
      (func)();
    }
  }

  // rb_do_thread
  // ------------
  // Spawns a ruby thread to execute the given function.
  // Returns the created ruby thread as a VALUE
  VALUE thread(RUBYDO_BLOCK thread_body) {
    auto body_ptr = new RUBYDO_BLOCK(thread_body);
    return rb_thread_create(invoke_and_destroy_returning_qnil, body_ptr);
  }
}

#ifdef DEBUG
  using namespace std;
  using namespace rubydo;
  
  void test_thread();
  VALUE make_int_update_thread(int &var, int new_val);

  int main(int argc, char** argv) {
    rubydo::init(argc, argv);
    
    test_thread();

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
      
    rb_require("./test.rb");
  }

  void test_thread() {
    int var = 1;
    auto thread = make_int_update_thread(var, 2);
    rb_funcall(thread, rb_intern("join"), 0);
    cout << ((var == 2) ? "Succeeded" : "Failed") << ": Running a ruby thread" << endl;
  }
  
  VALUE make_int_update_thread(int &var, int new_val) {
    return rubydo::thread([&](){
      var = new_val;
    });
  }
#endif
