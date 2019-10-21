@ok
<?php
class Base {
  public function empty() {}
  public function factory(): self {
    return new self();
  }
}
class Derived extends Base {
}
$b = new Base();
$d = new Derived();
echo get_class($b->factory()), PHP_EOL;
echo get_class($d->factory()), PHP_EOL;
