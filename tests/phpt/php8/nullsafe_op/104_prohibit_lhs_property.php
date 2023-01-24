@kphp_should_fail kphp8
<?php

class C {
  public int $dd;
  function __construct() {
    $this->dd = 42;
  }

  function d() {
    var_dump('C::d()');
    return $this->dd;
  }
}

class B {
  public C $cc;
  function __construct() {
    $this->cc = new C;
  }

  function c() {
    var_dump('B::c()');
    return $this->cc;
  }
}

$b = new B;
$b?->cc = new C;
