@ok
<?php

interface I {
    public function foo();
}
class A implements I {
    public function foo() {}
}
class B implements I {
    public function foo() {}
}

function foo() {
    var_dump("foo");
    return true ? new A() : new B();
}

function bar() {
    if (foo() instanceof A) {
        var_dump("A");
        return;
    } else if (foo() instanceof B) {
        var_dump("B");
        return;
    }
    var_dump("wtf");
}

bar();

