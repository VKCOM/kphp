@ok access
<?php

class Base {
  static protected $field = [];
  static protected $field2 = [];

  static protected function fun1() { echo "Base::fun1\n";}
  static protected function fun2() { echo "Base::fun2\n";}

  static public function test_setup() {
    static::fun1();
    static::fun2();
    static::$field[] = 1;
    static::$field2[] = 2;
  }
  static public function test_run() {
    var_dump(static::$field);
    var_dump(static::$field2);
  }
}

class Derived extends Base {
  static protected $field = [];

  static protected function fun1() { echo "Base::fun1\n";}

  static public function test_setup() {
    parent::test_setup();
    parent::$field[] = 3;
    parent::$field2[] = 4;
  }
}

Base::test_setup();
Derived::test_setup();
Base::test_run();
Derived::test_run();
