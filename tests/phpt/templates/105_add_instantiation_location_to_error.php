@kphp_should_fail
/in foo<any>/
/instantiated at/
<?php

class A { var $data = 10; }
class B { var $data = "asdf"; }

/**
 * @kphp-template $x
 */
function foo($x) {
    var_dump($x->data);
}

function bar() {
    foo(123);
}

function baz() {
    foo("asdf");
}

foo(new A());
foo(new B());
bar();
baz();
