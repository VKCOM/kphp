@kphp_should_fail
<?php

require_once 'kphp_tester_include.php';

class A implements Classes\IDo {
    public function do_it($a) {}
}

