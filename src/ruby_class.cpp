#include "rubydo.h"
#include "rubydo/ruby_class.h"
#include <string>
#include <iostream>

using namespace std;

namespace rubydo {

  // Constructs a RubyClass from an existing ruby class object.
  // Initializes the method lookup table if it doesn't exist.
  RubyClass::RubyClass (VALUE rb_class) : RubyModule(rb_class)  {
    superclass = rb_funcall(rb_class, rb_intern("superclass"), 0);
  }
  
  RubyClass
  RubyClass::define (VALUE rb_class) {
    return RubyClass(rb_class);
  }
  
  // Constructs a new RubyClass. If the specified class name describes
  // an existing class, it is re-opened, otherwise it is created.
  RubyClass::RubyClass (std::string name, VALUE superclass) : RubyModule(name) {
    this->superclass = superclass;
    init_rb_module();
  }
  
  RubyClass
  RubyClass::define (std::string name, VALUE superclass) {
    RubyClass ruby_class(name, superclass);
    ruby_class.init_rb_module();
    return ruby_class;
  }

  // Constructs a new RubyClass under the given outter_module (a module or class VALUE). 
  // If the specified class name describes an existing class, it is re-opened, otherwise it is created.
  RubyClass::RubyClass (VALUE outter_module, std::string name, VALUE superclass) : RubyModule(outter_module, name) {
    this->superclass = superclass;
  }
  
  RubyClass
  RubyClass::define (VALUE outter_module, std::string name, VALUE superclass) {
    RubyClass ruby_class(outter_module, name, superclass);
    ruby_class.init_rb_module();
    return ruby_class;
  }
  
  void
  RubyClass::rb_define_self () {
    // Module doesn't exist yet, define it
    if (RTEST(outter_module)) {
      self = rb_define_class_under(outter_module, name.c_str(), superclass);
    } else {
      self = rb_define_class(name.c_str(), superclass);
    }
  }
}