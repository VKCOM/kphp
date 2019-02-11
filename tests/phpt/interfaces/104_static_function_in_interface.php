@kphp_should_fail
<?php
interface I1 {
  static function do1();
}

class A implements I1 {
  function do1() {
    echo 'A do1', "\n";
  }
}
