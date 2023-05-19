@ok php8
<?php

enum Foo {
    case Bar;
}

enum Baz {
    case Qux;
}

var_dump(Foo::Bar instanceof Foo);
var_dump(Baz::Qux instanceof Baz);

var_dump(Foo::Bar instanceof Baz);
var_dump(Baz::Qux instanceof Foo);
