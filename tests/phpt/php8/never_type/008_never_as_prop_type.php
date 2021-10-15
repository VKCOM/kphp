@kphp_should_fail php8
/never cannot be used as a class property type/
<?php

class Foo {
  public never $int;
}

$foo = new Foo();
