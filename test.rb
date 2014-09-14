def test(desc)
  print desc
  if yield
    puts "Succeeded"
  else
    puts "Failed"
   end
end

begin
  test "RubydoTest class definition: " do
    Object.const_defined?(:RubydoTest) && RubydoTest.class == Class
  end
  
  test "Instance method definition: " do
    ruby_do_test = RubydoTest.new
    ruby_do_test.methods.any? { |m| m.to_s == "test_method_returns_success"}
  end
  
  test "Instance method return value: " do
    ruby_do_test = RubydoTest.new
    result = ruby_do_test.test_method_returns_success
    result == "success"
  end
  
  test "NestClass definition: " do
    RubydoTest.const_defined?(:NestedClass) && RubydoTest::NestedClass.class == Class
  end
  
  test "Nested instance method return value: " do
    nested_class_object = RubydoTest::NestedClass.new
    result = nested_class_object.nested_class_method
    result == "success"
  end
  
  test "Re-opening Object class by VALUE: " do
    obj = Object.new
    obj.rubydo_monkey_patch_by_value == "success"
  end
  
  test "Re-opening Object class by name: " do
    obj = Object.new
    obj.rubydo_monkey_patch_by_name == "success"
  end
  
rescue Exception => ex
  puts ex
  puts ex.backtrace
end