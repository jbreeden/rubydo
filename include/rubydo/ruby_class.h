#ifndef RUBYCLASS_H
#define RUBYCLASS_H

#include "ruby.h"
#include "rubydo.h"
#include "rubydo/ruby_module.h"
#include <string>
#include <functional>

namespace rubydo {
  class RubyClass : public RubyModule {
  public:
    VALUE superclass = Qnil;

    static RubyClass define (VALUE rb_class);
    static RubyClass define (std::string name, VALUE superclass = rb_cObject);
    static RubyClass define (VALUE outter_module, std::string name, VALUE superclass = rb_cObject);
    
  protected:
    RubyClass (VALUE rb_class);
    RubyClass (std::string name, VALUE superclass = rb_cObject);
    RubyClass (VALUE outter_module, std::string name, VALUE superclass = rb_cObject);
    virtual void rb_define_self();
  };
}
#endif