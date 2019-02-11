@kphp_should_fail
<?php

interface IDo2 {
    public function do_it($a, $b);
    public function fun();
}

class A implements IDo2 {
    public function do_it($a, $b) { }
}
