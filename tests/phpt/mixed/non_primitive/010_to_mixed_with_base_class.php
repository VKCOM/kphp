@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    public int $data;
    public function __construct(int $x) {
        $this->data = $x;
    }
    public function print() {
        echo "In A::print. data = " . $this->data . "\n";
    }
};

interface Fooable {
    public function foo();
}

class B extends A implements Fooable {
    public string $str_data;
    public function __construct(int $x, string $y) {
        parent::__construct($x);
        $this->str_data = $y;
    }
    public function print() {
        echo "In B::print. data = " . $this->data . "; str_data = " . $this->str_data . "\n";
    }
    public function foo() {
        echo "In B::foo.\n";
    }
};

/** @param mixed $x */
function check($x) {
    if ($x instanceof A) {
        echo "instanceof A\n";
        $x->print();
    }
    if ($x instanceof B) {
        echo "instanceof B\n";
        $x->print();
        $x->foo();
    }
    if ($x instanceof Fooable) {
        echo "instanceof C\n";
        $x->foo();
    }
}

/** @param A $a
 *  @return mixed
*/
function convert_from_a($a) {
    return to_mixed($a);
}

/** @param Fooable $obj
 *  @return mixed
*/
function convert_from_Fooable($obj) {
    return to_mixed($obj);
}

function main() {
    check(convert_from_a(new A(42)));
    check(convert_from_a(new B(200, "OK")));
    check(convert_from_Fooable(new B(404, "Not Found")));
}

main();
