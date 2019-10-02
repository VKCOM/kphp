@kphp_should_fail
/Can't access private const/
<?php

class A {
  private const A_PRIVATE_VAR = 40;
}

class B {
  public const B_PUBLIC_VAR = A::A_PRIVATE_VAR + 2;

  function foo() {
    var_dump(B::B_PUBLIC_VAR);
  }
}


$a = new B;
$a->foo();
