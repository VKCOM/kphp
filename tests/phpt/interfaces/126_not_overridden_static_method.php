@kphp_should_fail
/class: A must be abstract, method: II::make is not overridden/
<?php

interface II {
  static public function make();
}

class A implements II {}

$x = new A();
