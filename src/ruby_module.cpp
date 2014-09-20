#include "rubydo.h"
#include "rubydo/ruby_module.h"
#include "rubydo/ruby_class.h"
#include <string>
#include <iostream>

using namespace std;
using namespace rubydo;

namespace {
  RubyModule::MethodDetails
  get_current_method_details (VALUE &self) {
    RubyModule::MethodDetails result;
    
    VALUE method_sym = rb_funcall(rb_mKernel, rb_intern("__method__"), 0);
    result.name = rb_funcall(method_sym, rb_intern("to_s"), 0);
    VALUE rb_method = rb_funcall(self, rb_intern("method"), 1, method_sym);
    result.owner = rb_funcall(rb_method, rb_intern("owner"), 0);
    
    return result;
  }
}

namespace rubydo {

  // Static Members
  // --------------
  
  const string 
  RubyModule::instance_method_lookup_table_iv_name = "rubydo_instance_methods";
  
  const string 
  RubyModule::singleton_method_lookup_table_iv_name =  "rubydo_singleton_methods";
  
  VALUE 
  RubyModule::cRubydoMethod = Qnil;
  
  VALUE 
  RubyModule::invoke_instance_method(int argc, VALUE* argv, VALUE self) {
    MethodDetails method_details = get_current_method_details(self);
    
    // Retrieve instance method implementation from lookup table of this object's class
    VALUE lookup_table = rb_iv_get(method_details.owner, rubydo::RubyModule::instance_method_lookup_table_iv_name.c_str());
    VALUE boxed_method_wrapper = rb_funcall(lookup_table, rb_intern("[]"), 1, method_details.name);
    
    //if (!RTEST(boxed_method_wrapper)) {
    // TODO: Raise?
    //}
    
    rubydo::RubyModule::MethodWrapper* method_wrapper_ptr;
    Data_Get_Struct(boxed_method_wrapper, rubydo::RubyModule::MethodWrapper, method_wrapper_ptr);
    
    // TODO: try/catch around this and raise a ruby exception to
    return method_wrapper_ptr->implementation(self, argc, argv);
  }
  
  VALUE 
  RubyModule::invoke_singleton_method (int argc, VALUE* argv, VALUE self) {
    MethodDetails method_details = get_current_method_details(self);
    
    // Retrieve singleton method implementation from this object
    VALUE lookup_table = rb_iv_get(self, rubydo::RubyModule::singleton_method_lookup_table_iv_name.c_str());
    VALUE boxed_method_wrapper = rb_funcall(lookup_table, rb_intern("[]"), 1, method_details.name);
    
    rubydo::RubyModule::MethodWrapper* method_wrapper_ptr;
    Data_Get_Struct(boxed_method_wrapper, rubydo::RubyModule::MethodWrapper, method_wrapper_ptr);
    
    // TODO: try/catch around this and raise a ruby exception to
    return method_wrapper_ptr->implementation(self, argc, argv);
  }

  // Instance Members
  // ----------------
  
  // Constructs a RubyModule from an existing ruby module object.
  // Initializes the method lookup table if it doesn't exist.
  RubyModule::RubyModule (VALUE rb_module) {
    this->self = rb_module;
    VALUE name = rb_funcall(rb_module, rb_intern("name"), 0); 
    this->name = std::string(StringValuePtr(name));
    
    init_lookup_tables();
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
    
    init_lookup_tables();
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
  RubyModule::init_lookup_tables () {
    if (!RTEST(get_instance_method_lookup_table())) {
      init_instance_method_lookup_table();
    }
    
    if (!RTEST(get_singleton_method_lookup_table())) {
      init_singleton_method_lookup_table();
    }
  }
  
  void 
  RubyModule::init_instance_method_lookup_table () {
    VALUE lookup_table = rb_funcall(rb_cHash, rb_intern("new"), 0);
    rb_iv_set(self, RubyModule::instance_method_lookup_table_iv_name.c_str(), lookup_table);
  }
  
  void 
  RubyModule::init_singleton_method_lookup_table () {
    VALUE lookup_table = rb_funcall(rb_cHash, rb_intern("new"), 0);
    rb_iv_set(self, RubyModule::singleton_method_lookup_table_iv_name.c_str(), lookup_table);
  }

  VALUE 
  RubyModule::get_instance_method_lookup_table () {
    return rb_iv_get(self, RubyModule::instance_method_lookup_table_iv_name.c_str());
  }
  
  VALUE 
  RubyModule::get_singleton_method_lookup_table () {
    return rb_iv_get(self, RubyModule::singleton_method_lookup_table_iv_name.c_str());
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

  RubyModule& 
  RubyModule::define_method (std::string name, Method method) {
    rb_define_method(self, name.c_str(), RubyModule::invoke_instance_method, -1); /* -1 => send argc & argv */
    
    // Wrap implementation in struct
    RubyModule::MethodWrapper* method_wrapper_ptr = new RubyModule::MethodWrapper();
    method_wrapper_ptr->implementation = method;
    
    // Box MethodWrapper stuct in Ruby object
    VALUE ruby_wrapped_method = Data_Wrap_Struct(RubyModule::cRubydoMethod, NULL, internal::deleter<RubyModule::MethodWrapper*>, method_wrapper_ptr);
    
    // Add method to lookup table for self
    VALUE lookup_table = get_instance_method_lookup_table();
    VALUE method_name_string = rb_str_new_cstr(name.c_str());
    rb_funcall(lookup_table, rb_intern("[]="), 2, method_name_string, ruby_wrapped_method);
    
    return *this;
  }

  RubyModule& 
  RubyModule::define_singleton_method (std::string name, Method method) {
    rb_define_singleton_method(self, name.c_str(), RubyModule::invoke_singleton_method, -1); /* -1 => send argc & argv */
    
    // Wrap implementation in struct
    RubyModule::MethodWrapper* method_wrapper_ptr = new RubyModule::MethodWrapper();
    method_wrapper_ptr->implementation = method;
    
    // Box MethodWrapper stuct in Ruby object
    VALUE ruby_wrapped_method = Data_Wrap_Struct(RubyModule::cRubydoMethod, NULL, internal::deleter<RubyModule::MethodWrapper*>, method_wrapper_ptr);
    
    // Add method to lookup table for self
    VALUE lookup_table = get_singleton_method_lookup_table();
    VALUE method_name_string = rb_str_new_cstr(name.c_str());
    rb_funcall(lookup_table, rb_intern("[]="), 2, method_name_string, ruby_wrapped_method);
    
    return *this;
  }
  
}
