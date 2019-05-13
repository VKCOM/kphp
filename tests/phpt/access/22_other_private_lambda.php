@kphp_should_fail access
/Can't access/
<?php

class Other {
  static private function fun1() { 
    echo "Other::fun1\n";
  }

}

class Base {
  static private $field = [];
  static private function fun1() { 
    echo "Base::fun1\n";
  }
  static public function test() {
    $f = function() {
      Other::fun1();
    };
    $f();
  }
}

Base::test();
