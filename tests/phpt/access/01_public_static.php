@ok access
<?php

class Base {
  static public $field = [];
  static public $field2 = [];

  static public function fun1() { echo "Base::fun1\n";}
  static public function fun2() { echo "Base::fun2\n";}
}

class Derived extends Base {
  static public $field = [];

  static public function fun1() { echo "Base::fun1\n";}
}

Base::fun1();
Base::fun2();
Derived::fun1();
Derived::fun2();
Base::$field[] = 1;
Base::$field2[] = 2;
Derived::$field[] = 3;
Derived::$field2[] = 4;

var_dump(Base::$field);
var_dump(Base::$field2);
var_dump(Derived::$field);
var_dump(Derived::$field2);

