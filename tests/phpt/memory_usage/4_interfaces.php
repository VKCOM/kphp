@ok
<?php

interface IMyClass {
}

class MyClassEmpty implements IMyClass {
  // vtable 8
  // counter 4 + unique index 4 // aligned 8

  // total size = 16
  public function __construct() {}
}

class MyClass1 implements IMyClass {
  // vtable 8
  // counter 4 + unique index 4
  public $x = 123; // 8

  // total size = 24
}

class MyClass2 implements IMyClass {
  // vtable 8
  // counter 4 + unique index 4
  public $x = "hello"; // 8
  public $y = false; // 1 aligned 8
  public $z = 5; // 8

  // total size = 48
}

function test_empty_class() {
#ifndef KPHP
  var_dump(16);
  return;
#endif
  var_dump(estimate_memory_usage(new MyClassEmpty()));
}

function test_interfaces() {
#ifndef KPHP
  var_dump(24);
  var_dump(40);
  var_dump(16);
  var_dump(24);
  var_dump(40);
  var_dump(0);
  var_dump(16 + 24 + 40 + 80);
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
