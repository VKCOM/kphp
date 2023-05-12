@ok php8

<?php

enum Foo {
    case Bar;
    case Baz;
}

$var = Foo::Bar;

var_dump(
    match ($var) {
        Foo::Bar => "Bar",
        Foo::Baz => "Baz",
        default => "unreachable",
    }
);

$var = Foo::Bar;
var_dump(
    match ($var) {
        Foo::Bar => "Bar",
        Foo::Baz => "Baz",
        default => "unreachable",
    }
);