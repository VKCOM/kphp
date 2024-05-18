@ok php8
<?php

enum Foo {
    case Bar;
    case Baz;
    public function dump() {
        var_dump($this->name);
    }
}

const CONST_VAR = Foo::Bar;
$var = Foo::Baz;

CONST_VAR->dump();
$var->dump();
