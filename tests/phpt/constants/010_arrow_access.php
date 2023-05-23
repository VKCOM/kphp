@ok php8
<?php
require_once 'kphp_tester_include.php';

class B {
    static int $static_int = 0;
    public int $value;
    public function __construct(int $x) {
        B::$static_int += 1;
        $this->value = $x;
    }
    public function getValue() {
        return $this->value;
    }
}

const b = new B(42);

echo b->getValue();
echo b->value;

