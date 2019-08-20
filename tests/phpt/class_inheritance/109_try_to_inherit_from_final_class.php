@kphp_should_fail
/extends final class/
<?php

final class Base {
    public function foo() {
    }
}

class Derived extends Base {
}

