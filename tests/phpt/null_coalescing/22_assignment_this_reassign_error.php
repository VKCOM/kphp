@kphp_should_fail
/Modification of const variable: this/
<?php

class Test {
    public function foobar() {
        $this = new Test();
    }
}

// make sure that Test and foobar are not eliminated as a dead code
$x = new Test();
$x->foobar();
