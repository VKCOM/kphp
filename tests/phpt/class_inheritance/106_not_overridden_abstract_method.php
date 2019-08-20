@kphp_should_fail
/You should override/
<?php

abstract class Base {
    abstract public function foo();
}

class Derived extends Base {
}
