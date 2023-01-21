@ok php8
<?php

enum Foo {
    case Bar;
    case Baz;
    public function dump() {
        var_dump($this->name);
    }
    const ONE = '1';
}

Foo::Bar->dump();
Foo::Baz->dump();
var_dump(Foo::ONE);