#include "rubydo.h"
#include "ruby.h"
#include "ruby/thread.h"

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

  void invoke_and_destroy(void* arg) {
    if (arg != NULL) {
      DO_BLOCK *block = (DO_BLOCK*)arg;
      (**block)();
      delete block;
    }
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

namespace ruby {

  // init
  // ----
  // Initializes the ruby vm, and requires a few ruby modules
  // needed to allow some of the standard libraries to load correctly.
  // ----
  void init(int argc, char** argv) {
    ruby_sysinit(&argc, &argv);
    RUBY_INIT_STACK;
    ruby_init();
    ruby_init_loadpath();
    rb_require("enc/encdb");
    rb_require("enc/trans/transdb");
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
  //    ruby::without_gvl( DO [&]() {
  //      /* GVL is released, this code will execute in parallel to any ruby threads */
  //      result = some_rb_string;
  //    } END, DO [](){ /* unblock */ } );
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
  //    ruby::with_gvl DO [&]() {
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
  //      VALUE thread = ruby::thread DO [&, message] () {
  //       rb_funcall(rb_mKernel, rb_intern("puts"), 1, message);
  //      } END;
  //
  //      cout << "Thread created" << endl;
  //      return thread;
  //    }
  //
  //    int main (int argc, char** argv) {
  //      ruby::init(argc, argv);
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
