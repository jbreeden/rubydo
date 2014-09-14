#ifndef RUBYCLASS_H
#define RUBYCLASS_H

#include "ruby.h"
#include <string>
#include <functional>

namespace rubydo {
  class RubyClass {
  public:
    std::string name;
    VALUE rb_class = Qnil;
    VALUE superclass = Qnil;
    VALUE outter_module = Qnil;

    RubyClass(VALUE rb_class);
    RubyClass(std::string name, VALUE superclass = rb_cObject);
    RubyClass(VALUE outter_module, std::string name, VALUE superclass = rb_cObject);
    
    RubyClass define_class(std::string name, VALUE superclass = rb_cObject);
    void define_method(std::string name, rubydo::Method method_implementation);
    
    VALUE get_method_lookup_table();
    
  private:
    void init_rb_class();
    void init_method_lookup_table();

  };
}
#endif