@kphp_should_fail
<?php

class GrandPa {
    public $x = "sadf";

    public function fun() {
        var_dump($this->x);
    }
}

class AnotherPa extends GrandPa {
    public function fun() {
        var_dump($this->x);
    }
}

class Pa extends GrandPa {
    public $y = "zxcv";

    public function fun() {
        AnotherPa::fun();
    }
}

$d = new Pa();
$d->fun();

