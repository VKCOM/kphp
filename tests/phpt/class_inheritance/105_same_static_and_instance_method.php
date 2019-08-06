@kphp_should_fail
<?php

class B {
    public function foo() {
    }
}

class D extends B {
    public static function foo() {
    }
}

