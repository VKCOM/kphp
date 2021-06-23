@ok php7_4
<?php

function nonnull_bar(Bar $bar) {}

class Foo {
  public ?Bar $bar = null;

  public function __construct() {
    $this->bar = new Bar();
    $this->bar->value = 5342;
  }

  public function mutate() {
    $this->bar = null;
  }
}

class Bar {
  public $value;
}

function test() {
  // $foo->bar stops being non-null after the mutate() call,
  // but we don't catch this case during the compile-time yet.
  $foo = new Foo();
  if ($foo->bar) {
    var_dump($foo->bar->value);
    $old_value = $foo->bar->value;
    $foo->mutate();
    var_dump($foo->bar->value == $old_value);
  }
}

test();
