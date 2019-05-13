@ok access
<?php

class Base {
  static private $field = [];

  static private function fun1() { echo "Base::fun1\n";}

  static public function test() {
    self::fun1();
    self::$field[] = 1;
    var_dump(self::$field);
  }
}

Base::test();
