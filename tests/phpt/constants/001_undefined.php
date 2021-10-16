@kphp_should_fail
/Undefined constant 'Foo::UNDEFINED'/
/Undefined constant 'UndefinedClass::UNDEFINED'/
/Undefined constant 'UNDEFINED_CONSTANT'/
<?php

const SOME_NAME = "foo";

class Foo {
    const NAME = "foo";
}

function foo() {
    echo Foo::NAME;
    echo Foo::UNDEFINED;
    echo UndefinedClass::UNDEFINED;

    echo SOME_NAME;
    echo UNDEFINED_CONSTANT;
}

foo();
