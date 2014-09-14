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

  namespace internal {
    VALUE cRubydoMethod;
  
    VALUE invoke_instance_method(int argc, VALUE* argv, VALUE self) {
      // Determine method being called
      VALUE method_sym = rb_funcall(rb_mKernel, rb_intern("__method__"), 0);
      VALUE method_name = rb_funcall(method_sym, rb_intern("to_s"), 0);
      
      // Retrieve method implementation from lookup table of this object's class
      VALUE rbClass = rb_funcall(self, rb_intern("class"), 0);
      VALUE lookup_table = rb_iv_get(rbClass, rubydo::internal::method_lookup_table_iv_name);
      VALUE method = rb_funcall(lookup_table, rb_intern("[]"), 1, method_name);
      
      if (!RTEST(method)) {
        // TODO: Raise
      }
      
      rubydo::internal::MethodWrapper* method_wrapper_ptr;
      Data_Get_Struct(method, rubydo::internal::MethodWrapper, method_wrapper_ptr);
      
      // TODO: try/catch around this and raise a ruby exception to
      return method_wrapper_ptr->implementation(self, argc, argv);
    }
  }

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
    rubydo::internal::cRubydoMethod = rb_define_class("RubydoMethod", rb_cObject);
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
 
 int main(int argc, char** argv) {
  rubydo::init(argc, argv, false);

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
  
  cout << "Loading test.rb" << endl;
  rb_require("./test.rb");
 }
#endif