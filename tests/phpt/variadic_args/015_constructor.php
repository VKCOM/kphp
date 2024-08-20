@ok k2_skip
<?php

class A {
  /** @var any[] */
  public $args;
  /**
   * @param int[] $args
   */
  public function __construct(...$args) {
    var_dump(!!$this);
    var_dump($args);
    $this->args = $args;
  }
};

$x = new A(1, 2, 3);
var_dump($x->args);
