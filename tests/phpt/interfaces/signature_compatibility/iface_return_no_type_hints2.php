@ok
<?php

// It's allowed to add incompatible return type hints
// if the base type method doesn't specify it.

interface Iface {
  public function f($x);
}

class Impl implements Iface {
  public function f($x): void {}
}

class Impl2 implements Iface {
  public function f($x): int { return 10; }
}

/** @var Iface[] $list */
$list = [new Impl(), new Impl2()];
