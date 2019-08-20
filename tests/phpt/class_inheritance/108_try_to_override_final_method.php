@kphp_should_fail
/override final method/
<?php

class Base {
    final public function foo() {
    }
}

class Derived extends Base {
    public function foo() {
    }
}


