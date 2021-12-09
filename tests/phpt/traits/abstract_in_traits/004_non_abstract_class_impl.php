@ok
<?php

trait A {
    public function run(int $x) { var_dump($x); }
}

abstract class C {
    use A;
    abstract public function run();
}

class CD extends C {
    public function run() { var_dump("CD"); }
}

$cd = new CD();
$cd->run();
