@kphp_should_fail
<?php

trait Tr {
    public function foo() {}
}

var_dump(Tr::class);
