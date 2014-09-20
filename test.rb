def test(desc)
  if yield
    puts "Succeeded: #{desc}"
  else
    puts "Failed: #{desc}"
  end
rescue
  puts "Failed: #{desc}"
  raise
end

begin
  test "Module definition" do
    RubydoModule.class == Module
  end

  test "Class definition" do
    RubydoClass.class == Class
  end
  
  test "Defining singleton method on a module" do
    "success" == RubydoModule.module_singleton_method
  end
  
  test "Defining singleton method on a class" do
    "success" == RubydoClass.class_singleton_method
  end
  
  test "Defining an instance method on a class" do
    rubydo_object = RubydoClass.new
    "success" == rubydo_object.class_instance_method
  end
  
  test "Defining an instance method on a module" do
    class RubydoClass
      include RubydoModule 
    end
    rubydo_object = RubydoClass.new
    "success" == rubydo_object.module_instance_method
  end
  
  test "Class nested inside class" do
    RubydoClass.const_defined?(:NestedClass) && RubydoClass::NestedClass.class == Class
  end
  
  test "Nested class instance method definition" do
    nested_class_object = RubydoClass::NestedClass.new
    "success" == nested_class_object.nested_class_method
  end
  
  test "Re-opening Object class by VALUE" do
    obj = Object.new
    obj.rubydo_monkey_patch_by_value == "success"
  end
  
  test "Re-opening Object class by name" do
    obj = Object.new
    obj.rubydo_monkey_patch_by_name == "success"
  end
  
  test "Inheriting rubydo class instance method" do
    class Subclass < RubydoClass; end
    s = Subclass.new
    "success" == s.class_instance_method
  end
  
  test "Class definition under module" do
    RubydoModule::ClassUnderModule.class == Class
  end
  
  test "Mixed module & class nesting" do
    Mod1.class == Module &&
      Mod1::Class1.class == Class &&
      Mod1::Class1::Mod2.class == Module &&
      Mod1::Class1::Mod2::Class2.class == Class
  end
  
  test "Re-opening a nested class and adding a method" do
    c2 = Mod1::Class1::Mod2::Class2.new
    c2.deeply_nested_method == "success"
  end
  
rescue Exception => ex
  puts ex
  puts ex.backtrace
end
