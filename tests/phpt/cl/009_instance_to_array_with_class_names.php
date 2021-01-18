@ok
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
function check_instance_to_array_with_class_names($a, $b)
{
  echo "======= checking " . get_class($a) . " and " . get_class($b) . " ======= \n";
  $arr_a = instance_to_array($a);
  $arr_b = instance_to_array($b);

  if (kphp === 1) {
    var_dump($arr_a === $arr_b);
  } else {
    echo "bool(true)\n";
  }

  $arr_a = instance_to_array($a, true);
  $arr_b = instance_to_array($b, true);
  var_dump($arr_a);
  var_dump($arr_b);

  if (kphp === 1) {
    var_dump($arr_a === $arr_b);
  } else {
    echo "bool(false)\n";
  }
}

check_instance_to_array_with_class_names(new Empty1, new Empty2);
check_instance_to_array_with_class_names(new A1, new A2);
check_instance_to_array_with_class_names(new WithInner(new A1), new WithInner(new A2));
