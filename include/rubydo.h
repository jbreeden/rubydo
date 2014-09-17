#ifndef RUBYDO_H
#define RUBYDO_H

// ruby.h conflicts with some other library headers,
// including wx.h. Defining RUBYDO_NO_CONFLICTS will
// avoid pulling in ruby.h, exposing a reduced interface
// to rubydo in order to avoid the conflicts.
#ifndef RUBYDO_NO_CONFLICTS
#include "ruby.h"
#endif

#include <functional>
#include <memory>

// DO_BLOCK Macros
// TODO: Could probably do this without the shared_pts's
#define DO_BLOCK std::shared_ptr<std::function<void()>>
#define DO (DO_BLOCK(new std::function<void()>(
#define END )))

namespace rubydo {

  void init(int argc, char** argv, bool using_std_lib);
  void without_gvl(DO_BLOCK, DO_BLOCK);
  void with_gvl(DO_BLOCK);

#ifndef RUBYDO_NO_CONFLICTS

  typedef std::function<VALUE(VALUE self, int argc, VALUE* argv)> Method;

  namespace internal {
    
    // Internal Data
    // ------------
    
    const char method_lookup_table_iv_name[] = "rubydo_methods";
      
    // The Ruby class that will a MethodWrapper is held in when inserted into the method lookup table
    extern VALUE cRubydoMethod;
    
    // A struct type used to hold Method objects so we can Data_Wrap_Struct them
    struct MethodWrapper {
      Method implementation;
    };
    
    // Internal Helper Functions
    // ------------------------
    
    VALUE invoke_instance_method(int argc, VALUE* argv, VALUE self);
    
    template <class PtrType>
    void deleter (PtrType ptr) {
      delete ptr;
    }
  }

  VALUE thread(DO_BLOCK);
#endif
}

#endif
