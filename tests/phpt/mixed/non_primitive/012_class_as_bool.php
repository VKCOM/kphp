@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    public int $x;
    public function __construct(int $y) {
        $this->$x = $y;
    }
}

/** @param mixed $a */
function check($a) {
    if ($a) {
        echo "Not Null!\n";
    } else {
        echo "Null!\n";
    }
}

function main() {
    /** @var mixed */
    $a = to_mixed(new A(123));
    check($a);
    if (1 + 1 === 2) {
        $a = null;
    }
    check($a);
    if (1 + 2 == 4) {
        $a = to_mixed(new A(42));
    }
    check($a);
}

main();
