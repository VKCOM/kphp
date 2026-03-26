@kphp_should_warn
/Dangerous undefined behaviour unset, \[foreach var = \$arr\]/
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

try {
  throw new MyException2();
} catch (MyException1 $e1) {
} catch (MyException2 $e2) {
  $arr = ['something' => 1];
  foreach ($arr as &$item) {
    unset($arr['something']);
  }
}
