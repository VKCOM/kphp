@kphp_should_fail
/You may not use trait directly/
<?php

trait Tr {
    public static function foo() {
        var_dump("asdf");
    }
}

Tr::foo();
