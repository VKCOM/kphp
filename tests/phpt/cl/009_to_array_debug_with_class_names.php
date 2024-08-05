@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class Empty1
{

}

class Empty2
{

}

interface IA
{

}

class A1 implements IA
{
  public $field = "hello";
}

class A2 implements IA
{
  public $field = "hello";
}

class WithInner
{
  /**
   * @var IA
   */
  public $inner = null;
  public $field = "hello";

  /**
   * @param IA $inner
   */
  public function __construct(IA $inner)
  {
    $this->inner = $inner;
  }
}


/**
 * @kphp-template $a
 * @kphp-template $b
 * @param object $a
 * @param object $b
 */
function check_to_array_debug_with_class_names($a, $b)
{
  echo "======= checking " . get_class($a) . " and " . get_class($b) . " ======= \n";
  $arr_a = to_array_debug($a);
  $arr_b = to_array_debug($b);

  if (kphp === 1) {
    var_dump($arr_a === $arr_b);
  } else {
    echo "bool(true)\n";
  }

  $arr_a = to_array_debug($a, true);
  $arr_b = to_array_debug($b, true);
  var_dump($arr_a);
  var_dump($arr_b);

  if (kphp === 1) {
    var_dump($arr_a === $arr_b);
  } else {
    echo "bool(false)\n";
  }
}

check_to_array_debug_with_class_names(new Empty1, new Empty2);
check_to_array_debug_with_class_names(new A1, new A2);
check_to_array_debug_with_class_names(new WithInner(new A1), new WithInner(new A2));
