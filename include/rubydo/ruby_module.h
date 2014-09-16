#ifndef RUBYDOMODULE_H
#define RUBYDOMODULE_H

#include "ruby.h"

namespace rubydo {
  
  class RubyClass;
  
  class RubyModule {
  public:
    static RubyModule define(VALUE rb_module);
    static RubyModule define(std::string name);
    static RubyModule define(VALUE outter_module, std::string name);
  
    // Name of the module
    std::string name;
    
    // The actual Ruby VALUE of the defined module
    VALUE self = Qnil;
    
    // The containing outter module
    VALUE outter_module = Qnil;
    
    RubyModule define_module(std::string name);
    RubyClass define_class(std::string name, VALUE superclass = rb_cObject);
    void define_method(std::string name, rubydo::Method method_implementation);
    void define_singleton_method(std::string name, rubydo::Method method_implementation);
    
    VALUE get_method_lookup_table();
    
  protected:

    RubyModule(VALUE rb_module);
    RubyModule(std::string name);
    RubyModule(VALUE outter_module, std::string name);
    
    void init_rb_module();
    
    // Defines this module in the ruby world
    // (Overridden by RubyClass)
    virtual void rb_define_self();
    
  private:
    // TODO: Need instance & singleton method lookups to avoid name collisions
    void init_method_lookup_table();
  };
}

#endif