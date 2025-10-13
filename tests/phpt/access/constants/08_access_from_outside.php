@kphp_should_fail
/Can't access/
<?php
class A {
  private const PRIV = 42;
  protected const PROT = 42;
}

echo A::PRIV;
echo A::PROT;
