@ok
<?php
require_once 'kphp_tester_include.php';

class JustBase {
    public array $just_base_data;
    public function __construct() {
        $this->just_base_data = [1, "data", 42.42];
    }
    public function print() {
        var_dump($this->just_base_data);
    }
}

class IntBase extends JustBase {
    public int $int_base_data1;
    public int $int_base_data2;
    public function __construct() {
        parent::__construct();
        $this->int_base_data1 = 123;
        $this->int_base_data2 = 456;
    }
    public function print() {
        parent::print();
        var_dump($this->int_base_data1);
        var_dump($this->int_base_data2);
    }
}

class ID1 extends IntBase {
    public string $s;
    public function __construct(string $s2) {
        parent::__construct();
        $this->s = $s2;
    }
    public function print() {
        parent::print();
        echo "ID1::print and s = " . $this->s . "\n";
    }
}

class ID2 extends IntBase {
    public string $s1;
    public string $s2;
    public string $s3;
    public string $s4;

    public function __construct(string $p) {
        parent::__construct();
        $this->s1 = $p . " first";
        $this->s2 = $p . " second";
        $this->s3 = $p . " third";
        $this->s4 = $p . " fourth";
    }
    public function print() {
        parent::print();
        echo "ID2::print and s1 = " . $this->s1 . " and s2 = ". $this->s2 . "and s3 = " . $this->s3 . "and s4 = " . $this->s4 ."\n";
    }
}

class StringBase extends JustBase {
    public string $string_base_data1;
    public string $string_base_data2;
    public function __construct() {
        parent::__construct();
        $this->string_base_data1 = "lupa";
        $this->string_base_data2 = "pupa";
    }
    public function print() {
        parent::print();
        var_dump($this->string_base_data1);
        var_dump($this->string_base_data2);
    }
}

class SD1 extends StringBase {
    public int $kek;
    public float $lol;
    public function __construct(int $x, float $y) {
        parent::__construct();
        $this->kek = $x;
        $this->lol = $y;
    }
    public function print() {
        parent::print();
        var_dump($this->kek);
        var_dump($this->lol);
    }
}

class SD2 extends StringBase {
    public function __construct() {
        parent::__construct();
    }
    public function print() {
        parent::print();
        echo "From SD2::print\n";
    }
}

/** @param mixed $m */
function check($m) {
    if ($m instanceof JustBase) {
        echo "instanceof JustBase!\n";
        $m->print();
    }
    if ($m instanceof IntBase) {
        echo "instanceof IntBase!\n";
        $m->print();
    }
    if ($m instanceof ID1) {
        echo "instanceof ID1!\n";
        $m->print();
    }
    if ($m instanceof ID2) {
        echo "instanceof ID2!\n";
        $m->print();
    }
    if ($m instanceof StringBase) {
        echo "instanceof StringBase!\n";
        $m->print();
    }
    if ($m instanceof SD1) {
        echo "instanceof SD1!\n";
        $m->print();
    }
    if ($m instanceof SD2) {
        echo "instanceof SD2!\n";
        $m->print();
    }
}

function main() {
    check(to_mixed(new JustBase()));
    check(to_mixed(new IntBase()));
    check(to_mixed(new ID1("kitten")));
    check(to_mixed(new ID2("puppy")));
    check(to_mixed(new StringBase()));
    check(to_mixed(new SD1(1234, 4.04)));
    check(to_mixed(new SD2()));

}

main();
