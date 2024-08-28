@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class BB {
    public ?Classes\F $f = null;
    /** @var ?Classes\KeywordAsFieldName */
    private $k = null;

    function f() {
        if ($this->f) $this->f->appendInt(1);
        else echo "f null\n";
        if ($this->k) $this->k->int++;
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

/**
 * @param ?\Classes\Z3Infer $b
 */
function f3($b) {
  if ($b) $b->thisHasInfer(1,2);
}

f1(null);
f2();
f3(null);
(new BB)->f();
