@kphp_should_fail
/you may not use trait directly/
<?php

trait Tr {
    public static function foo() {
        var_dump("asdf");
    }
}

Tr::foo();
