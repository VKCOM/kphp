@ok
<?php
class Foo {
    private $field = "";

    public function __construct(string $a) {
        $this->field = $a;
    }

    public function __toString(): string {
        return $this->field . "\n";
    }
}

function test1(string $val) {
    echo $val;
}

function test() {
    $f = new Foo("Foo class 1");

    echo $f;
    print $f;

    $f1 = new Foo((string)$f);
    print $f1;

    test1((string)$f);
}

test();
