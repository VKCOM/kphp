@ok php8
<?php

enum Foo {
    case Bar;
    case Baz;

    public function dump() {
        var_dump($this->name);
    }
}

var_dump(Foo::Bar === Foo::Baz);
var_dump(Foo::Bar !== Foo::Baz);

var_dump(Foo::Baz === Foo::Bar);
var_dump(Foo::Baz !== Foo::Bar);