@ok php8
<?php

enum Foo {
    case Bar;
    case Baz;
    public function dump() {
        var_dump($this->name);
    }
}

function print_Foo(Foo $var) {
    $var->dump();
}

print_Foo(Foo::Bar);
print_Foo(Foo::Baz);
