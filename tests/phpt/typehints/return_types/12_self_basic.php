@ok
<?php
class Foo {
  /**
   * @return Foo
   */
  public static function factory(): self {
    return new self();
  }

  public function bar() {
  }
}

$foo = Foo::factory();
$foo->bar();

