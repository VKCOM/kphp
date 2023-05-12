@ok php8
<?php

enum Foo {
    case Bar;
    case Baz;
}

enum Single {
    case Single;
}

enum EmptyEnum {
}


var_dump(count(Foo::cases()));
var_dump(count(Single::cases()));
var_dump(count(EmptyEnum::cases()));