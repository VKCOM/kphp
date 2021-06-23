@ok php7_4
<?php

function nonnull_foo(Foo $foo) {}
function nonnull_bar(Bar $bar) {}

class Foo {
  public Bar $bar;
  public ?Bar $nullable_bar;
  public ?Foo $nullable_foo;

  public function __construct(Foo $foo = null) {
    $this->bar = new Bar();
    $this->nullable_bar = new Bar();
    $this->nullable_foo = $foo;
  }

  public function test_implicit() {
    if ($this->nullable_bar) {
      nonnull_bar($this->nullable_bar);
    }

    if (!$this->nullable_bar) {
    } else {
      nonnull_bar($this->nullable_bar);
    }
  }

  public function test_early_return() {
    if ($this->nullable_bar === null) {
      return;
    }
    if ($this->nullable_foo === null) {
      return;
    }
    nonnull_bar($this->nullable_bar);
    nonnull_foo($this->nullable_foo);
  }
}

class Bar {}

$foo = new Foo(new Foo(null));

$foo->test_implicit();
$foo->test_early_return();
