@ok
<?php

class A {
  protected const A_PROTECTED_VAR = 2;
}

class B extends A {
  public const B_PUBLIC_VAR = 40 + A::A_PROTECTED_VAR;

  function foo() {
    var_dump(B::B_PUBLIC_VAR);
  }
}


$a = new B;
$a->foo();
