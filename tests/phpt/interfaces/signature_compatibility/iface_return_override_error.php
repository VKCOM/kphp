@kphp_should_fail
/Declaration of C::f\(\) must be compatible with Iface1::f\(\)/
<?php

interface Iface1 {
  public function f(): int;
}

interface Iface2 extends Iface1 {
  public function f(): string;
}

class C implements Iface2 {
  public function f(): string { return 0; }
}

$c = new C();
