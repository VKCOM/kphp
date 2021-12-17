@kphp_should_fail
/You may not use trait directly/
<?php

trait Tr {
    public function foo() {
    }
}

$tr = new Tr();
