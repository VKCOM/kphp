@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    public int $data;
    public function __construct(int $x) {
        $this->data = $x;
    }
    public function print() {
        echo "A::data = " . $this->data . "\n";
    }
}

function foo() {
    $a = to_mixed(new A(123));
    if ($a instanceof A) {
        $a->print();
    }
}

foo();
