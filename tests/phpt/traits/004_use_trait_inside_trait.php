@ok
<?php

trait A {
    public function foo() {
        var_dump("foo");
    }
}

trait AB {
    use A;
    public function bar() {
        var_dump("bar");
    }
}

class C {
    use AB;
}

$ab = new C();
$ab->foo();
$ab->bar();
