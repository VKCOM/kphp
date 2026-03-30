@kphp_should_warn
/Dangerous undefined behaviour unset, \[foreach var = \$arr\]/
<?php

try {
  throw new Exception();
} catch (Exception $e) {
  $arr = ['something' => 1];
  foreach ($arr as &$item) {
    unset($arr['something']);
  }
}
