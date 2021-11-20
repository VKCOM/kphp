@ok
<?php

class Base {
  public function sayHello() {
    echo 'Hello ';
  }
}

trait SayWorld {
  public function sayHello() {
    parent::sayHello();
    echo 'World!';
  }
}

function test_extend_traits() {
  $obj = new class extends Base {
    use SayWorld;
  };
  $obj->sayHello();
}

test_extend_traits();
