@ok
<?php

interface IMyClass {
}

class MyClassEmpty implements IMyClass {
  // vtable 8
  // counter 4 // aligned 8

  // total size = 16
  public function __construct() {}
}

class MyClass1 implements IMyClass {
  // vtable 8
  // counter 4
  public $x = true; // 1 // cnt + $x aligned 8

  // total size = 16
}

class MyClass2 implements IMyClass {
  // vtable 8
  // counter 4 // aligned 8
  public $x = "hello"; // 8
  public $y = false; // 1
  public $z = 5; // 4 // $y + $z aligned 8

  // total size = 32
}

function test_empty_class() {
#ifndef KittenPHP
  var_dump(16);
  return;
#endif
  var_dump(estimate_memory_usage(new MyClassEmpty()));
}

function test_interfaces() {
#ifndef KittenPHP
  var_dump(16);
  var_dump(32);
  var_dump(16);
  var_dump(16);
  var_dump(32);
  var_dump(0);
  var_dump(16 + 16 + 32 + 8 + 72);
  return;
#endif
  var_dump(estimate_memory_usage(new MyClass1()));
  var_dump(estimate_memory_usage(new MyClass2()));

  $ix = [new MyClassEmpty(), new MyClass1(), new MyClass2(), null];
  var_dump(estimate_memory_usage($ix[0]));
  var_dump(estimate_memory_usage($ix[1]));
  var_dump(estimate_memory_usage($ix[2]));
  var_dump(estimate_memory_usage($ix[3]));
  var_dump(estimate_memory_usage($ix));
}

test_empty_class();
test_interfaces();
