@kphp_should_fail
<?php

require_once("polyfills.php");

class A implements Classes\IDo {
    public function do_it($a) {}
}

