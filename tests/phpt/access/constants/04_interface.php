@kphp_should_fail
/const access modifier in interface must be public/
<?php
interface IProtected {
  protected const PROT = 10;
}
interface IPrivate {
  private const PRIV = 42;
}

class A implements IProtected {
  public function empty() {};
}
class B implements IPrivate {
  public function empty() {};
}

$a = new A();
$b = new B();
