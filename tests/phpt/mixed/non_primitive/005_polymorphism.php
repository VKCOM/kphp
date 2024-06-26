@ok
<?php
require_once 'kphp_tester_include.php';

interface Printer {
    public function print();
}

interface Fooable {
    public function foo(int $x);
}

class A implements Printer, Fooable {
    public int $data;
    public function __construct(int $x) {
        $this->data = $x;
    }
    public function print() {
        echo "A::data = " . $this->data . "\n";
    }
    public function foo(int $x) {
        echo "From foo: A::data = " . $this->data . "; arg = " . $x . "\n";
    }
}

class X {
    public int $x_data = 0;
}

class B extends X {
    public function __construct(int $x) {
        $this->x_data = $x;
    }
}

class C extends X implements Printer, Fooable {
    public int $data;
    public function __construct(int $x) {
        $this->data = $x;
        $this->x_data = $x * 3 + 42;
    }
    public function print() {
        echo "C::print (" . $this->data . ", " . $this->x_data . ")\n";
    }
    public function foo(int $x) {
        echo "C::foo (" . $this->data . ", " . $this->x_data . ", " . $x .  ")\n";
    }
}

/** @param mixed $m */
function check($m) {
    if ($m instanceof Printer) {
        $m->print();
    }
    if ($m instanceof Fooable) {
        $m->foo(42);
    }
    if ($m instanceof X) {
        echo "X data = " . $m->x_data . "\n";
    }
}

function main() {
    check(to_mixed(new A(42)));
    check(to_mixed(new B(123)));
    check(to_mixed(new C(456)));
}

main();
