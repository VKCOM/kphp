<?php

function some_function1() {
  while(0);
}

function some_function2() {
  while(0);
}

class TestClass {
  static $static_field = null;

  static function static_function_example() {
    while(0);
  }
}

define('DEFINE_CONSTANT', null);

class BenchmarkMicro {
  static $N = 1000;

  function benchmarkEmptyLoop() {
    for ($i = 0; $i < self::$N; ++$i) {
    }
  }

  function benchmarkSimpleuCall() {
    for ($i = 0; $i < self::$N; $i++) {
      some_function1();
    }
  }

  function benchmarkSimpleudCall() {
    for ($i = 0; $i < self::$N; $i++) {
      some_function2();
    }
  }

  function benchmarkSimpleiCall() {
    for ($i = 0; $i < self::$N; $i++) {
      error_get_last();
    }
  }

  static $static_field = null;
  public $class_field = 0;
  const CLASS_CONSTANT = 0;

  function benchmarkReadStaticCl() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = self::$static_field;
    }
  }

  function benchmarkWriteStaticCl() {
    for ($i = 0; $i < self::$N; ++$i) {
      self::$static_field = 0;
    }
  }

  function benchmarkIssetStaticCl() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = isset(self::$static_field);
    }
  }

  function benchmarkEmptyStaticCl() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = empty(self::$static_field);
    }
  }

  static function static_function_example() {
  }

  function benchmarkCallStaticCl() {
    for ($i = 0; $i < self::$N; ++$i) {
      self::static_function_example();
    }
  }

  function benchmarkReadStatic() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = TestClass::$static_field;
    }
  }

  function benchmarkWriteStatic() {
    for ($i = 0; $i < self::$N; ++$i) {
      TestClass::$static_field = 0;
    }
  }

  function benchmarkIssetStatic() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = isset(TestClass::$static_field);
    }
  }

  function benchmarkEmptyStatic() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = empty(TestClass::$static_field);
    }
  }

  function benchmarkCallStatic() {
    for ($i = 0; $i < self::$N; ++$i) {
      TestClass::static_function_example();
    }
  }

  function benchmarkReadProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $this->class_field;
    }
  }

  function benchmarkWriteProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $this->class_field = 0;
    }
  }

  function benchmarkAssignAddProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $this->class_field += 2;
    }
  }

  function benchmarkPreIncProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      ++$this->class_field;
    }
  }

  function benchmarkPreDecProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      --$this->class_field;
    }
  }

  function benchmarkPostIncProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $this->class_field++;
    }
  }

  function benchmarkPostDecProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $this->class_field--;
    }
  }

  function benchmarkIssetProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = isset($this->class_field);
    }
  }

  function benchmarkEmptyProp() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = empty($this->class_field);
    }
  }

  function function_example() {
  }

  function benchmarkCallClass() {
    for ($i = 0; $i < self::$N; ++$i) {
      $this->function_example();
    }
  }

  function benchmarkReadConstClass() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = self::CLASS_CONSTANT;
    }
  }

  function benchmarkCreatObject() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = new TestClass();
    }
  }

  function benchmarkReadConst() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = DEFINE_CONSTANT;
    }
  }

  function benchmarkReadAutoGlobal() {
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $_GET;
    }
  }

  function benchmarkReadHash() {
    $hash = array('test' => 0);
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $hash['test'];
    }
  }

  function benchmarkReadStrOffset() {
    $str = "test";
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $str[1];
    }
  }

  function benchmarkIssetor() {
    $val = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $val ?: null;
    }
  }

  function benchmarkIssetor2() {
    $f = false;
    $j = 0;
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $f ?: $j + 1;
    }
  }

  function benchmarkTernary() {
    $val = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
    $f = false;
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $f ? null : $val;
    }
  }

  function benchmarkTernary2() {
    $f = 0;
    $j = 0;
    for ($i = 0; $i < self::$N; ++$i) {
      $x = $f ? $f : $j + 1;
    }
  }

  function benchmarkReadGlobal() {
    global $g_var;
    $g_var = 0 ? "hello" : [1, 2, 3];
    for ($i = 0; $i < self::$N; ++$i) {
      global $g_var;
      $x = $g_var;
    }
  }
}
