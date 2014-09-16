#include "rubydo.h"
#include "rubydo/ruby_module.h"
#include "rubydo/ruby_class.h"
#include <string>
#include <iostream>

using namespace std;

namespace rubydo {

  // Constructs a RubyModule from an existing ruby module object.
  // Initializes the method lookup table if it doesn't exist.
  RubyModule::RubyModule (VALUE rb_module) {
    this->self = rb_module;
    VALUE name = rb_funcall(rb_module, rb_intern("name"), 0); 
    this->name = std::string(StringValuePtr(name));
    
    if (!RTEST(get_method_lookup_table())) {
      init_method_lookup_table();
    }
  }
  
  RubyModule
  RubyModule::define (VALUE rb_module) {
    return RubyModule(rb_module);
  }

  // Constructs a new RubyModule. If the specified module name describes
  // an existing module, it is re-opened, otherwise it is created.
  RubyModule::RubyModule (std::string name) {
    this->name = name;
  }
  
  RubyModule
  RubyModule::define (std::string name) {
    RubyModule module(name);
    module.init_rb_module();
    return module;
  }

  // Constructs a new RubyModule under the given outter_module (a module or class VALUE). 
  // If the specified module name describes an existing module, it is re-opened, otherwise it is created.
  RubyModule::RubyModule (VALUE outter_module, std::string name) {
    this->name = name;
    this->outter_module = outter_module;
  }
  
  RubyModule
  RubyModule::define (VALUE outter_module, std::string name) {
    RubyModule module(outter_module, name);
    module.init_rb_module();
    return module;
  }

  void 
  RubyModule::init_rb_module () {
    // Determine where constant is defined for this module
    VALUE const_root;
    if (RTEST(outter_module)) {
      const_root = outter_module;
    } else {
      const_root = rb_cObject;
    }
    
    VALUE name_as_rb_string = rb_str_new_cstr(name.c_str());
    
    // Check for existing module
    VALUE const_exists = rb_funcall(const_root, rb_intern("const_defined?"), 1, name_as_rb_string);
    VALUE const_value = Qnil;
    if (const_exists) {
      const_value = rb_funcall(const_root, rb_intern("const_get"), 1, name_as_rb_string);
    }
    
    if (const_exists) {
      // Module already exists, no need to define self
      self = const_value;
    } else {
      // Module does 
      rb_define_self();
    }
    
    if (!RTEST(get_method_lookup_table())) {
      init_method_lookup_table();
    }
  }
  
  void
  RubyModule::rb_define_self () {
    // Module doesn't exist yet, define it
    if (RTEST(outter_module)) {
      self = rb_define_module_under(outter_module, name.c_str());
    } else {
      self = rb_define_module(name.c_str());
    }
  }

  void 
  RubyModule::init_method_lookup_table () {
    VALUE lookup_table = rb_funcall(rb_cHash, rb_intern("new"), 0);
    rb_iv_set(self, internal::method_lookup_table_iv_name, lookup_table);
  }

  VALUE 
  RubyModule::get_method_lookup_table () {
    return rb_iv_get(self, internal::method_lookup_table_iv_name);
  }
  
  RubyModule
  RubyModule::define_module (std::string name) {
    // Return a new module with the current module as the outter_module
    return RubyModule::define(self, name);
  }

  RubyClass 
  RubyModule::define_class (std::string name, VALUE superclass) {
    // Return a new class with the current module as the outter_module
    return RubyClass::define(self, name, superclass);
  }

  void 
  RubyModule::define_method (std::string name, Method method) {
    rb_define_method(self, name.c_str(), internal::invoke_instance_method, -1); /* -1 => send argc & argv */
    
    // Wrap implementation in struct
    internal::MethodWrapper* method_wrapper_ptr = new internal::MethodWrapper();
    method_wrapper_ptr->implementation = method;
    
    // Box MethodWrapper stuct in Ruby object
    VALUE ruby_wrapped_method = Data_Wrap_Struct(internal::cRubydoMethod, NULL, internal::deleter<internal::MethodWrapper*>, method_wrapper_ptr);
    
    // Add method to lookup table for self
    VALUE lookup_table = get_method_lookup_table();
    VALUE method_name_string = rb_str_new_cstr(name.c_str());
    rb_funcall(lookup_table, rb_intern("[]="), 2, method_name_string, ruby_wrapped_method);
  }

  void 
  RubyModule::define_singleton_method (std::string name, Method method_implementation) {

  }
  
}