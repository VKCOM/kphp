@kphp_should_fail
/class A must be abstract: method I1::do1 is not overridden/
<?php
interface I1 {
  static function do1();
}

class A implements I1 {
  function do1() {
    echo 'A do1', "\n";
  }
}
