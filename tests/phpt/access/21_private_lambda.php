@ok access
<?php

class Base {
  static private $field = [];
  static private function fun1() { 
    echo "Base::fun1\n";
  }
  static public function test() {
    $f = function() {
      Base::fun1();
    };
    $f();
  }
}

Base::test();
