@kphp_should_fail
/Redeclaration of Foo::__construct()/
<?php

enum Foo {
    case Bar;
    case Baz;
    function __construct() {
        return $this;
    }
};

var_dump(Foo::Bar->name);