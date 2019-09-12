@kphp_should_fail
/but this class is trait/
<?php

trait Tr {
    public function foo() {
    }
}

$tr = new Tr();
