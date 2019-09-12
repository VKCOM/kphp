@kphp_should_fail
/must be declared abstract/
<?php

trait Tr {
    abstract public function foo();
}

class A {
    use Tr;
}
