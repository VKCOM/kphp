@ok
<?php

function test_empty_class() {
  class EmptyClass {
    function __construct() {}
  }

#ifndef KPHP
  var_dump(0);
  var_dump(0);
  var_dump(0);
  return;
#endif
  var_dump(estimate_memory_usage(new EmptyClass()));
  var_dump(estimate_memory_usage(tuple(new EmptyClass(), 1.2)));

  /** @var EmptyClass */
  $x = null;
  var_dump(estimate_memory_usage($x));
}

function test_class_with_simple_fields() {
  class MyClass1 {
    public $x = 1;
    public $y = 1.0;
    public $z = true;
    public $u = "hello world";
    public $w = [1, 2, 3];
  }

#ifndef KPHP
  var_dump(40);
  var_dump(40);
  var_dump(0);
  return;
#endif

  var_dump(estimate_memory_usage(new MyClass1()));
  var_dump(estimate_memory_usage(tuple(new MyClass1(), false)));

  /** @var MyClass1 */
  $x = null;
  var_dump(estimate_memory_usage($x));
}

function test_class_with_dynamic_array() {
  class MyClass2 {
    /**
     * @param int $size
     */
    public function __construct($size) {
      $this->x = range(0, $size);
    }

    // counter 4 + unique index 4
    public $x = []; // 8
  } // total size = 16

#ifndef KPHP
  var_dump(240);
  var_dump(16 + 240);
  return;
#endif
  $instance = new MyClass2(10);
  var_dump(estimate_memory_usage($instance->$x));
  var_dump(estimate_memory_usage($instance));
}

function test_class_with_cyclic_dependence() {
  class ClassWithCyclicDependence {
    public int $value = 0;
    /** @var ClassWithCyclicDependence */
    public $child;

    public function __construct(int $value) {
      $this->value = $value;
      $this->child = $this;
    }
  }

#ifndef KPHP
  var_dump(24);
  return;
#endif
  $instance = new ClassWithCyclicDependence(42);
  var_dump(estimate_memory_usage($instance));
}

function test_class_with_cyclic_dependence_in_dynamic_array() {
  class ClassWithCyclicDependenceInArray {
  public int $value = 0; // 8
  /** @var ClassWithCyclicDependenceInArray[] */
  public $childs = []; // 8

  public function __construct(int $value) {
      $this->value = $value;
      $this->childs[] = $this;
    }
  }

#ifndef KPHP
  var_dump(72);
  return;
#endif
  $instance = new ClassWithCyclicDependenceInArray(42);
  var_dump(estimate_memory_usage($instance));
}

test_empty_class();
test_class_with_simple_fields();
test_class_with_dynamic_array();
test_class_with_cyclic_dependence();
test_class_with_cyclic_dependence_in_dynamic_array();
