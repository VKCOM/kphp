@kphp_should_fail
/Called to_mixed\(\) for class A, but it's not marked with @kphp-may-be-mixed/
<?php
require_once 'kphp_tester_include.php';

// Here should be @kphp-may-be-mixed tag
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
    $x = to_mixed(new A(42));
}

foo();