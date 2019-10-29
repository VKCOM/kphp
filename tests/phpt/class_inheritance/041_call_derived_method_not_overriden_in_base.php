@ok
<?php

class A {
    public function foo() {
        var_dump("A");
    }
}

class A0 extends A {
    public function foo() {
        var_dump("A0");
    }
}

class B extends A0 {
}

class C extends B {
}

/**@var A*/
$x = true ? new C() : new A();
$x->foo();
