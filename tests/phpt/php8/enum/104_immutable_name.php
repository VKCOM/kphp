@kphp_should_fail
/Modification of const variable: name/
/Modification of const variable: name/
<?php

enum Foo {
    case Bar;
    case Baz;
};

Foo::Bar->name = "kek";
Foo::Baz->name[0] = "s";