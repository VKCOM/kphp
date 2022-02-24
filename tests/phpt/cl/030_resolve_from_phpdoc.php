@ok
<?php
require_once 'kphp_tester_include.php';

class BB {
    public ?Classes\F $f = null;

    function f() {
        if ($this->f) $this->f->appendInt(1);
        else echo "f null\n";
    }
}

function f1(?Classes\E $e) {
    if ($e) $e->makeNextE();
    else echo "e null\n";
}

function f2() {
    /** @var ?Classes\A $a */
    $a = null;
    if ($a) $a->printA();
    else echo "a null\n";
}

f1(null);
f2();
(new BB)->f();
