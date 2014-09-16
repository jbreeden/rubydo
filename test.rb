def test(desc)
  print desc
  print ": "
  if yield
    puts "Succeeded"
  else
    puts "Failed"
  end
rescue
  puts "Failed"
  raise
end

begin
  test "Class definition" do
    RubydoClass.class == Class
  end
  
  test "Instance method definition" do
    ruby_do_test = RubydoClass.new
    result = ruby_do_test.test_method_returns_success
    result == "success"
  end
  
  test "Class nested inside class" do
    RubydoClass.const_defined?(:NestedClass) && RubydoClass::NestedClass.class == Class
  end
  
  test "Nested class instance method definition" do
    nested_class_object = RubydoClass::NestedClass.new
    result = nested_class_object.nested_class_method
    result == "success"
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
    s.test_method_returns_success == "success"
  end
  
  test "Module definition" do
    RubydoModule.class == Module
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
