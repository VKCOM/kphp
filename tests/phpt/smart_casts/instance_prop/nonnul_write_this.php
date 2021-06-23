@ok php7_4
<?php

function nonnull_foo(Foo $foo) {}

class Foo {}

class Bar {
  /** @var ?Foo */
  public $foo;

  public static function new_foo(): Foo {
    return new Foo();
  }

  public function get_foo(): Foo {
    if (!$this->foo) {
      $this->foo = new Foo();
      nonnull_foo($this->foo);
    }
    return $this->foo;
  }

  public function get_foo2(): Foo {
    if ($this->foo === null) {
      $this->foo = self::new_foo();
      nonnull_foo($this->foo);
    }
    return $this->foo;
  }
}

$bar = new Bar();
$bar->get_foo();
$bar->get_foo2();
