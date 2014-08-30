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


#define DO_BLOCK std::shared_ptr<std::function<void()>>
#define DO (DO_BLOCK(new std::function<void()>(
#define END )))

namespace ruby {
  void init(int argc, char** argv);
  void without_gvl(DO_BLOCK, DO_BLOCK);
  void with_gvl(DO_BLOCK);

#ifndef RUBYDO_NO_CONFLICTS
  VALUE thread(DO_BLOCK);
#endif
}

#endif

