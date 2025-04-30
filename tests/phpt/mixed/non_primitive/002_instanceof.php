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

class C {

}

/**
 * @param mixed $x
 */
function check_instance_of($x) {
    if ($x instanceof A) {
        echo "instanceof A\n";
    } else {
        echo "not instanceof A\n";
    }

    if ($x instanceof C) {
        echo "instanceof C\n";
    } else {
        echo "not instanceof C\n";
    }
}

function foo() {
    $obj_as_mixed = to_mixed(new A(123));
    check_instance_of($obj_as_mixed);

    /** @var mixed */
    $int_as_mixed = 42;
    check_instance_of($int_as_mixed);

    check_instance_of(to_mixed(new C));    
}

foo();
