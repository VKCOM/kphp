@ok
<?php

class B {
  public $b = 0;
}

class A {
    public $x = 10;
    /** @var B */
    public $b;
    /** @var B[] */
    public $b_arr = [];
    /** @var B[]|false */
    public $b_arr_or_false = false;

    function __construct() { $this->b = new B; }

    /** @return B */
    function getB() { return $this->b; }
}

/**
 * @kphp-infer
 * @param A[]|false $a
 */
function run($a) {
    foreach ($a as $key => $_) {
      $a[$key]->x = 20;
    }
    echo $a[0]->x, "\n";
    if(isset($a[0]->b_arr[0])) {
      echo $a[0]->b_arr[0]->b, "\n";
    }
    if(isset($a[0]->b_arr_or_false[0])) {
      $a[0]->b_arr_or_false[0]->b = 10;
      echo $a[0]->b_arr_or_false[0]->b, "\n";
    }
    $a[0]->b_arr_or_false[] = new B;
    $a[0]->b_arr_or_false[0]->b = 20;
    echo $a[0]->b_arr_or_false[0]->b, "\n";
    $a[0]->getB()->b = 30;
    echo $a[0]->getB()->b, "\n";
    var_dump($a[0]->x);
    $a[0]->getB()->b = 5;
    var_dump($a[0]->getB()->b);
}

run(true ? [new A] : false);
