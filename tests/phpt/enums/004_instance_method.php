@ok php8
<?php

enum Foo {
    case Bar;
    case Baz;

    public function dump() {
        var_dump($this->name);
    }
}

Foo::Bar->dump();
Foo::Baz->dump();
