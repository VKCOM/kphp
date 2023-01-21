@kphp_should_fail
/Redeclaration of function Foo::cases()/
<?php

enum Foo {
    case Bar;
    case Baz;
    function cases() {
        return 42;
    }
};

var_dump(Foo::Bar->name);