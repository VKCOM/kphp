@ok
<?php

// example from https://www.php.net/manual/en/language.oop5.anonymous.php
class Outer {
  private $prop = 1;
  protected $prop2 = 2;

  protected function func1() {
    return 3;
  }

  public function func2() {
    return new class($this->prop) extends Outer {
      private $prop3;

      public function __construct($prop) {
        $this->prop3 = $prop;
      }

      public function func3() {
        return $this->prop2 + $this->prop3 + $this->func1();
      }
    };
  }
}

function test_access_to_outer_class() {
  echo (new Outer)->func2()->func3();
}

test_access_to_outer_class();
