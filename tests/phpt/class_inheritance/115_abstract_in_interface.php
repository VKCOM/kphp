@kphp_should_fail
/abstract methods may not be declared in interfaces/
<?php

interface A {
  abstract public function foo();
}

