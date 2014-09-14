#include "rubydo.h"
#include "rubydo/ruby_class.h"
#include <string>

// Constructs a RubyClass from an existing ruby class object.
// Initializes the method lookup table if it doesn't exist.
rubydo::RubyClass::RubyClass (VALUE rb_class) {
  this->rb_class = rb_class;
  VALUE name = rb_funcall(rb_class, rb_intern("name"), 0); 
  this->name = std::string(StringValuePtr(name));
  
  this->superclass = rb_funcall(rb_class, rb_intern("superclass"), 0);
  
  if (!RTEST(get_method_lookup_table())) {
    init_method_lookup_table();
  }
}

// Constructs a new RubyClass. If the specified class name describes
// an existing class, it is re-opened, otherwise it is created.
rubydo::RubyClass::RubyClass (std::string name, VALUE superclass) {
  this->name = name;
  this->superclass = superclass;
  init_rb_class();
}

// Constructs a new RubyClass under the given outter_module (a module or class VALUE). 
// If the specified class name describes an existing class, it is re-opened, otherwise it is created.
rubydo::RubyClass::RubyClass (VALUE outter_module, std::string name, VALUE superclass) {
  this->name = name;
  this->superclass = superclass;
  this->outter_module = outter_module;
  init_rb_class();
}

void 
rubydo::RubyClass::init_rb_class () {
  // Determine where constant is defined for this class
  VALUE const_root;
  if (RTEST(outter_module)) {
    const_root = outter_module;
  } else {
    const_root = rb_cObject;
  }
  
  VALUE name_as_rb_string = rb_str_new_cstr(name.c_str());
  
  // Check for existing class
  VALUE const_exists = rb_funcall(const_root, rb_intern("const_defined?"), 1, name_as_rb_string);
  VALUE const_defines_class = Qfalse;
  VALUE const_value = Qnil;
  if (const_exists) {
    const_value = rb_funcall(const_root, rb_intern("const_get"), 1, name_as_rb_string);
    const_defines_class = (TYPE(const_value) == T_CLASS);
  }
  
  // Class already exists, no need to define it 
  // (TODO: superclass is ignored in this case, as you cannot change the superclass. Perhaps throw/warn?)
  if (const_defines_class) {
    rb_class = const_value;
  } else {
    // Class doesn't exist yet, define it
    if (RTEST(outter_module)) {
      rb_class = rb_define_class_under(outter_module, name.c_str(), superclass);
    } else {
      rb_class = rb_define_class(name.c_str(), superclass);
    }
  }
  
  if (!RTEST(get_method_lookup_table())) {
    init_method_lookup_table();
  }
}

void 
rubydo::RubyClass::init_method_lookup_table () {
  VALUE lookup_table = rb_funcall(rb_cHash, rb_intern("new"), 0);
  rb_iv_set(rb_class, rubydo::internal::method_lookup_table_iv_name, lookup_table);
}

VALUE 
rubydo::RubyClass::get_method_lookup_table () {
  return rb_iv_get(rb_class, rubydo::internal::method_lookup_table_iv_name);
}

rubydo::RubyClass 
rubydo::RubyClass::define_class (std::string name, VALUE superclass) {
  // Return a new class with the current class as the outter_module
  return RubyClass(this->rb_class, name, superclass);
}

void 
rubydo::RubyClass::define_method (std::string name, rubydo::Method method) {
  rb_define_method(rb_class, name.c_str(), rubydo::internal::invoke_instance_method, -1); /* -1 => send argc & argv */
  
  // Wrap implementation in struct
  rubydo::internal::MethodWrapper* method_wrapper_ptr = new rubydo::internal::MethodWrapper();
  method_wrapper_ptr->implementation = method;
  
  // Box MethodWrapper stuct in Ruby object
  VALUE ruby_wrapped_method = Data_Wrap_Struct(rubydo::internal::cRubydoMethod, NULL, rubydo::internal::deleter<rubydo::internal::MethodWrapper*>, method_wrapper_ptr);
  
  // Add method to lookup table for rb_class
  VALUE lookup_table = get_method_lookup_table();
  VALUE method_name_string = rb_str_new_cstr(name.c_str());
  rb_funcall(lookup_table, rb_intern("[]="), 2, method_name_string, ruby_wrapped_method);
}
