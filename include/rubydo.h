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

  void init(int argc, char** argv);
  void use_ruby_standard_library();
  void without_gvl(DO_BLOCK, DO_BLOCK);
  void with_gvl(DO_BLOCK);

#ifndef RUBYDO_NO_CONFLICTS
namespace internal {
    
    // Internal Helper Functions
    // ------------------------
    
    template <class PtrType>
    void deleter (PtrType ptr) {
      delete ptr;
    }
  }

  VALUE thread(DO_BLOCK);
#endif
}

#ifndef RUBYDO_NO_CONFLICTS
#include "rubydo/ruby_module.h"
#include "rubydo/ruby_class.h"
#endif

#endif
