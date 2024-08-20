@ok php8
<?php
require_once 'kphp_tester_include.php';

class B {
    public int $value;
    public function __construct(int $x) {
        // B::$static_int += 1;  // accessing globals in constructor in const classes crashes const_init; todo how to detect it in the future?
        $this->value = $x;
    }
    public function getValue() {
        return $this->value;
    }
}

const b = new B(42);

echo b->getValue();
echo b->value;

