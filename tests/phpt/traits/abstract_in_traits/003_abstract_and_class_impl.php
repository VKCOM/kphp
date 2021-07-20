@ok
<?php

trait A {
    abstract public function run($x, $y);
}

class C {
    use A;
    public function run() {
        var_dump("Ignore A::run");
    }
}

$c = new C();
$c->run();
