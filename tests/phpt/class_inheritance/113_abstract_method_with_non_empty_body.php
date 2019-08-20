@kphp_should_fail
/abstract methods must have empty body/
<?php

abstract class A {
    abstract public function foo() {}
}

