@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    public int $x;
    public function __construct(int $y) {
        $this->$x = $y;
    }
}

/** @param mixed $a
 *  @param mixed $b
 */
function check($a, $b) {
    if ($a === $b) {
        echo "EQ3!\n";
    } else {
        echo "NEQ3!\n";
    }
}

function main() {
    $x = new A(123);
    $y = to_mixed($x);
    /** @var mixed */
    $arr = [to_mixed(new A(42)), to_mixed(new A(52)), [to_mixed($x)], to_mixed($x),
            to_mixed($x), [$y], $y, null, [1, 2, 3], "abobus", "a"."b"];
    foreach ($arr as $item1) {
        foreach ($arr as $item2) {
            check($item1, $item2);
        }
    }
}

main();
