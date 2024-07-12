@ok k2_skip
<?php

class A {
  // vtable 8
  // counter 4 + unique index 4
  // total size = 16
  function __construct() {}
}

class B1 extends A {
  // vtable 8
  // counter 4 + unique index 4
  // total size = 16
  function __construct() {}
}

class B2 extends A {
  // vtable 8
  // counter 4 + unique index 4
  public $x = "hello"; // 8
  public $y = false; // 1 aligned 8
  // total size = 32
  function __construct() {}
}

class C1 extends B1 {
  // vtable 8
  // counter + unique index 4
  public $x = 123; // 8
  // total size = 24
  function __construct() {}
}

class C2 extends B2 {
  // parent 32
  public $z = true; // 1 aligned to parent
  // total size = 32
  function __construct() {}
}

function test_polymorphic_classes() {
#ifndef KPHP
  var_dump(16);
  var_dump(16);
  var_dump(32);
  var_dump(24);
  var_dump(32);
  return;
#endif
  var_dump(estimate_memory_usage(new A()));
  var_dump(estimate_memory_usage(new B1()));
  var_dump(estimate_memory_usage(new B2()));
  var_dump(estimate_memory_usage(new C1()));
  var_dump(estimate_memory_usage(new C2()));
}

function test_polymorphic_classes_in_array() {
#ifndef KPHP
  var_dump(16);
  var_dump(16);
  var_dump(32);
  var_dump(24);
  var_dump(32);
  var_dump(0);
  var_dump(16 + 16 + 32 + 24 + 32 + 96);
  return;
#endif
  $ix = [new A(), new B1(), new B2(), new C1(), new C2(), null];
  var_dump(estimate_memory_usage($ix[0]));
  var_dump(estimate_memory_usage($ix[1]));
  var_dump(estimate_memory_usage($ix[2]));
  var_dump(estimate_memory_usage($ix[3]));
  var_dump(estimate_memory_usage($ix[4]));
  var_dump(estimate_memory_usage($ix[5]));
  var_dump(estimate_memory_usage($ix));
}

test_polymorphic_classes();
test_polymorphic_classes_in_array();
