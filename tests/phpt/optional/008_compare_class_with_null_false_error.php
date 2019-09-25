@kphp_should_fail
/Got 'equals' operation between A and false\|null types/
<?php

class A {
  public $x = 1;
}

/**
 * @kphp-infer
 * @param A $lhs
 * @param null|false $rhs
 */
function compare_class_with_null_false($lhs, $rhs) {
  var_dump($lhs === $rhs);
}

compare_class_with_null_false(false, null);
