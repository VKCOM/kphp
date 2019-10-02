@ok
<?php

class EmptyClass {
  public function __construct () {
    echo "EmptyClass ctr\n";
  }
}

function clone_empty_class_test() {
  $a = new EmptyClass();
  $b = clone $a;
  var_dump($b === $a);
}

function clone_empty_lambda_test() {
  $a = function () {
    echo "empty lambda!\n";
  };

  $b = clone $a;
  $b();
  var_dump($b === $a);
}

clone_empty_class_test();
clone_empty_lambda_test();
