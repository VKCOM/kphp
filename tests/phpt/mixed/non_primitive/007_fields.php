@ok
<?php
require_once 'kphp_tester_include.php';

class IntWrapper {
    public int $x;
    public function __construct(int $y) {
        $this->x = $y;
    }
}

class A {
    static $static_data = [1, 2, 5];
    public  $data;
    public function __construct($x) {
        $this->data = $x;
    }
}

function traverse($m) {
    if ($m instanceof IntWrapper) {
        echo "data = " . $m->x . "\n";
    }
    if (is_array($m)) {
        foreach ($m as $inner) {
            traverse($inner);
        }
    }
}

function check(A $a) {
    traverse($a->data);
    traverse(A::$static_data);
}

function main() {
    A::$static_data = 52;
    check(new A(123));
    check(new A(to_mixed(new IntWrapper(432))));
    A::$static_data = [[[[to_mixed(new IntWrapper(12)), ["pppp" => to_mixed(new IntWrapper(123)), [1, 2, 3]]], null]], to_mixed(new IntWrapper(555))];
    check(new A("abobus"));
    check(new A([[[[to_mixed(new IntWrapper(555)), to_mixed(new IntWrapper(444))]], to_mixed(new IntWrapper(11111))]]));
}

main();
