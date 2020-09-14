@ok
<?php

require_once 'kphp_tester_include.php';

interface ContainsX {
    /**
     * @kphp-infer
     * @param int $x
     */
    public function set_x($x);
    /**
     * @kphp-infer
     * @return int
     */
    public function get_x();
}

class X1 implements ContainsX {
    public $x = 10;

    /**
     * @kphp-infer
     * @param int $x
     */
    public function set_x($x) { $this->x = $x; }
    /**
     * @kphp-infer
     * @return int
     */
    public function get_x() { return $this->x; }
}

class X2 implements ContainsX {
    public $x = 1000;

    /**
     * @kphp-infer
     * @param int $x
     */
    public function set_x($x) { $this->x = $x; }
    /**
     * @kphp-infer
     * @return int
     */
    public function get_x() { return $this->x; }
}

/**
 * @param ContainsX $ido
 * @return ContainsX
 */
function clone_ContainsX(ContainsX $ido) {
    if ($ido instanceof X1) {
        return clone instance_cast($ido, X1::class);
    } else {
        return clone instance_cast($ido, X2::class);
    }
}

function test_clone() {
    /** @var ContainsX $a */
    $a = new X1();
    $a = new X2();
    var_dump($a->get_x());

    $a2 = clone_ContainsX($a);
    $a2->set_x(9999);
    var_dump("diff:" . $a->get_x());
    var_dump("diff:" . $a2->get_x());
}

function test_without_clone() {
    /** @var ContainsX $a */
    $a = new X1();
    $a = new X2();
    var_dump($a->get_x());

    /** @var ContainsX $a2 */
    $a2 = $a;
    $a2->set_x(9999);
    var_dump("same:" . $a->get_x());
    var_dump("same:" . $a2->get_x());
}

test_clone();
test_without_clone();

