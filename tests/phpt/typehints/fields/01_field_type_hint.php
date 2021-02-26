@ok php7_4
<?php

class A {
    public int $i;
    private ?string $s = '';
    public B $b;

    public function __construct(int $i) {
        $this->i = $i;
        $this->b = new B($this);
    }
}

class B {
    static int $count = 0;

    var ?A $a = null;

    public function __construct(?A $a) {
        $this->a = $a;
        self::$count++;
    }
}

function demo() {
    $a1 = new A(10);
    echo $a1->i, "\n";
    echo $a1->b->a->i, "\n";
    echo $a1->b->a->b->a->i, "\n";

    $a2 = new A(20);
    echo $a2->b->a->i, "\n";

    echo B::$count;
}

demo();
