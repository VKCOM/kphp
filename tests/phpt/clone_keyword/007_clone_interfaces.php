@ok k2_skip
<?php

#ifndef KPHP
function instance_cast($c, $s) { return $c; }
function get_reference_counter($c) { return 0; }
#endif

interface IPrintable {
    function print_me();
    function change_me();
    /**
     * @param int $value
     */
    function check_refcnt($value);
}

class A implements IPrintable {
    public $x = 10;

    function print_me() { var_dump($this->x); }
    function change_me() { $this->x = 100; }
    /**
     * @param int $value
     */
    function check_refcnt($value) {
        #ifndef KPHP
        var_dump(true);
        return;
        #endif
        var_dump($value == get_reference_counter($this) - 1);
    }

    function __clone() { var_dump("clone A: {$this->x}"); }
}

class B implements IPrintable {
    public $y = 40;

    function print_me() { var_dump($this->y); }
    function change_me() { $this->y = 100; }
    /**
     * @param int $value
     */
    function check_refcnt($value) {
        #ifndef KPHP
        var_dump(true);
        return;
        #endif
        var_dump($value == get_reference_counter($this) - 1);
    }

    function __clone() { var_dump("clone B: {$this->y}"); }
}

function test(IPrintable $c, IPrintable $c_clone) {
    $c_clone->change_me();

    $c->print_me();
    $c_clone->print_me();

    $c->check_refcnt(1);
    $c_clone->check_refcnt(1);
}

function run() {
    /** @var IPrintable $c */
    $c = new A();
    $c_clone = clone $c;
    $c_clone->check_refcnt(1);
    test($c, $c_clone);
    $c_clone->check_refcnt(1);

    $c = new B();
    $c_clone = clone $c;
    test($c, $c_clone);
}

run();
