@ok
<?php

interface Iface1 {
  public function foo(): int;
}

interface Iface2 extends Iface1 {
  public function foo(): int;
}

class C implements Iface2 {
  public function foo(): int { return 0; }
}

$c = new C();
