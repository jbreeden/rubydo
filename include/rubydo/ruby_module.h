#ifndef RUBYDOMODULE_H
#define RUBYDOMODULE_H

#include "ruby.h"
#include "rubydo.h"
#include <string>

namespace rubydo {
  
  class RubyClass;
  
  
  class RubyModule {
    friend void rubydo::init(int argc, char** argv, bool using_std_lib);
  
    // Nested Types
    // -------------
    
  public:
  
    typedef std::function<VALUE(VALUE self, int argc, VALUE* argv)> Method;
    
    // A struct type used to hold Method objects so we can Data_Wrap_Struct them
    struct MethodWrapper {
      Method implementation;
    };
    
    // Struct representing method call information for implementation lookup
    struct MethodDetails {
      VALUE owner;
      VALUE name;
    };
  
    // Static Members
    // ---------------
    
  protected:
    static const std::string instance_method_lookup_table_iv_name;
    static const std::string singleton_method_lookup_table_iv_name;
  
    // Initialized in rubydo::init
    static VALUE cRubydoMethod;
  
    static VALUE invoke_instance_method(int argc, VALUE* argv, VALUE self);
    static VALUE invoke_singleton_method(int argc, VALUE* argv, VALUE self);
  
    // Instance Members
    // ------------------
  
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
    
    // define_method returns the ruby module to allow method chaining
    RubyModule& define_method(std::string name, Method method);
    RubyModule& define_singleton_method(std::string name, Method method);
    
    VALUE get_instance_method_lookup_table();
    VALUE get_singleton_method_lookup_table();
    
  protected:

    RubyModule(VALUE rb_module);
    RubyModule(std::string name);
    RubyModule(VALUE outter_module, std::string name);
    
    void init_rb_module();
    
    // Defines this module in the ruby world
    // (Overridden by RubyClass)
    virtual void rb_define_self();
    
  private:
    void init_lookup_tables();
    void init_instance_method_lookup_table();
    void init_singleton_method_lookup_table();
  };
}

#endif