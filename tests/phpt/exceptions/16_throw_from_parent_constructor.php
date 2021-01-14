@ok
<?php

class A {
  public $x = 0;

  public function __construct() {
    $this->x = 15;
    if ($this->x) {
      throw new Exception();
    }
  }
}

class B extends A {
  public $y = 0;

  public function __construct() {
    $this->y = 10;
    try {
      parent::__construct();
    } catch(Exception $e) {
      var_dump($e->getLine());
    }
    var_dump($this->y);
    try {
      $_ = new A();
    } catch(Exception $e) {
      var_dump($e->getLine());
    }
    var_dump($this->y);
  }
}

$db = new B();
