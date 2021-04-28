@kphp_should_fail
/Declaration of Bar::testChildClass\(\) must be compatible with Foo::testChildClass\(\)/
<?php

// Zend/tests/type_declarations/parameter_type_variance.phpt

class Foo {
    function testParentClass(Foo $foo) {}
    function testBothClass(Foo $foo) {}
    function testChildClass(Foo $foo) {}
    function testNoneClass(Foo $foo) {}
}

class Bar extends Foo {
    function testParentClass(Foo $foo) {}
    function testBothClass(Foo $foo) {}
    function testChildClass(Bar $foo) {}
    function testNoneClass(Foo $foo) {}
}
