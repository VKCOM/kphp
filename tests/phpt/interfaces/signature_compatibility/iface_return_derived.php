@ok
<?php

class A {}
class B extends A {}

interface Iface {
  public function f(): A;
}

class Impl implements Iface {
  public function f(): B { return new B(); }
}

/** @var Iface[] $list */
$list = [new Impl()];


interface I {
    /** @return static */
    static public function fromArray(array $data);
}

abstract class Not implements I {
    static public function fromArray(array $data): ?Not { return static::getMe(); }
    abstract static public function getMe(): Not;
    public function m() { echo "Not\n"; }
}

class D1 extends Not {
    static public function getMe(): Not { return new self; }
    public function m() { echo "m1\n"; }
}
class D2 extends Not {
    static public function getMe(): Not { return new self; }
    public function m() { echo "m2\n"; }
}
class D3 extends Not {
    static public function getMe(): Not { return new self; }
    public function m() { echo "m3\n"; }
}

D1::fromArray([1])->m();
D2::fromArray([1])->m();
D3::fromArray([1])->m();
