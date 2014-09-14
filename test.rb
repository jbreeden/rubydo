def test(desc, cond)
  print desc
  if cond
    puts "Succeeded"
  else
    puts "Failed"
   end
end

begin
  test("RubydoTest class definition: ", Object.const_defined?(:RubydoTest) && RubydoTest.class == Class)
  
  ruby_do_test = RubydoTest.new
  
  test("Instance method definition: ", ruby_do_test.methods.any? { |m| m.to_s == "test_method_returns_success"})
  
  result = ruby_do_test.test_method_returns_success
  
  test("Instance method return value: ", result == "success")
  
  test("NestClass definition: ", RubydoTest.const_defined?(:NestedClass) && RubydoTest::NestedClass.class == Class)
  
  nested_class_object = RubydoTest::NestedClass.new
  result = nested_class_object.nested_class_method
  
  test("Nested instance method return value: ", result == "success")
  
  obj = Object.new
  
  test("Re-opening Object class by VALUE: ", obj.rubydo_monkey_patch_by_value == "success")
  test("Re-opening Object class by name: ", obj.rubydo_monkey_patch_by_name == "success")
  
rescue Exception => ex
  puts ex
  puts ex.backtrace
end