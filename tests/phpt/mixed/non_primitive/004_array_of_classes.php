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

function foo() {
    $x = to_mixed(new A(0));
    $arr = [to_mixed(new A(123)), $x, 123, "abcd", [null, $x], $x, to_mixed(new A(456))];
    foreach($arr as $item) {
        if ($item instanceof A) {
            $item->print();
        }
        if (is_array($item)) {
            foreach ($item as $inner_item) {
                if ($inner_item instanceof A) {
                    $inner_item->print();
                }
            }
        }
    }
}

foo();
