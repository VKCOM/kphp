@kphp_should_fail
<?php

interface IOne {
    public function one($x);
}

interface ITwo extends IOne {
    public function two($x, $y);
}

class ImplTwo implements ITwo {
    public function two($x, $y) { var_dump($x + $y); }
}

