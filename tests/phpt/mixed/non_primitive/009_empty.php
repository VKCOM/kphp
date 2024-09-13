@ok
<?php
require_once 'kphp_tester_include.php';

class EmptyNoHierarchy {
    public function print() {
        echo "From EmptyNonHerarchy\n";
    }
}

class EmptyBase {
    public function print() {
        echo "From EmptyBase\n";
    }
}

class NonEmptyBase {
    public int $data = 42;
    public function __construct(int $x) {
        $this->data = $x;
    }
    public function print() {
        echo "NonEmptyBase::data = " . $this->data . "\n";
    }
}

class EmptyFromEmpty extends EmptyBase {
    public function print() {
        echo "EmptyFromEmpty\n";
    }
}

class EmptyFromNonEmpty extends NonEmptyBase {
    public function print() {
        echo "EmptyFromNonEmpty::";
        parent::print();
    }
}

class NonEmptyFromEmpty extends EmptyBase {
    public string $str;
    public function __construct(string $x) {
        $this->str = $x;
    }
    public function print() {
        echo "EmptyFromNonEmpty::str = " . $this->str . "\n";
        parent::print();
    }
}

class NonEmptyFromNonEmpty extends NonEmptyBase {
    public string $str;
    public function __construct(string $x) {
        $this->str = $x;
    }
    public function print() {
        echo "NonEmptyFromNonEmpty::str = " . $this->str . "\n";
        parent::print();
    }
}

/** @param mixed $m */
function check($m) {
    if ($m instanceof EmptyNoHierarchy) {
        echo "instanceof EmptyNoHierarchy!\n";
        $m->print();
    }
    if ($m instanceof EmptyBase) {
        echo "instanceof EmptyBase!\n";
        $m->print();
    }
    if ($m instanceof NonEmptyBase) {
        echo "instanceof NonEmptyBase!\n";
        $m->print();
    }
    if ($m instanceof EmptyFromEmpty) {
        echo "instanceof EmptyFromEmpty!\n";
        $m->print();
    }
    if ($m instanceof NonEmptyFromEmpty) {
        echo "instanceof NonEmptyFromEmpty!\n";
        $m->print();
    }
    if ($m instanceof EmptyFromNonEmpty) {
        echo "instanceof EmptyFromNonEmpty!\n";
        $m->print();
    }
    if ($m instanceof NonEmptyFromNonEmpty) {
        echo "instanceof NonEmptyFromNonEmpty!\n";
        $m->print();
    }
}

function foo() {
    check(to_mixed(new EmptyNoHierarchy()));
    check(to_mixed(new EmptyBase()));
    check(to_mixed(new NonEmptyBase(0)));
    check(to_mixed(new EmptyFromEmpty()));
    check(to_mixed(new EmptyFromNonEmpty(42)));
    check(to_mixed(new NonEmptyFromEmpty("lupa")));
    check(to_mixed(new NonEmptyFromNonEmpty("pupa")));
}

foo();
