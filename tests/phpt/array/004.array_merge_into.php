@ok
<?php
require_once 'kphp_tester_include.php';

class A {
  public $a = 0;

  /**
   * @param int $a
   */
  function __construct($a = 0) {
    $this->a = $a;
  }
}

function test_array_merge_into_same_type() {
  $a = [1,2,3];
  array_merge_into($a, [4, 5]);
  var_dump($a);

  $b = ['1','2','3'];
  array_merge_into($b, ['4', '5']);
  $b_clone = array_slice($b, 0);
  array_merge_into($b, $b_clone);
  array_merge_into($b, ['6', '7']);
  var_dump($b);
}

function test_array_merge_into_changing_type() {
  $a = [1,2,3];
  #ifndef KPHP
  $a = [1.0,2.0,3.0];
  #endif
  array_merge_into($a, [4.1, 5.2]);
  var_dump($a);

  $b = ['1','2','3'];
  array_merge_into($b, [4, 5]);
  $b_clone = array_slice($b, 0);
  array_merge_into($b, $b_clone);
  array_merge_into($b, [['6'], ['7']]);
  var_dump($b);
}

function test_array_merge_into_instances() {
  /** @var A[] */
  $a = [];
  array_merge_into($a, [new A(1)]);
  array_merge_into($a, [new A(2), new A(3)]);
  foreach ($a as $item) {
    echo "->a = ", $item->a, "\n";
  }
}

test_array_merge_into_same_type();
test_array_merge_into_changing_type();
test_array_merge_into_instances();
