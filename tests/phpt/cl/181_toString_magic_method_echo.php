@ok
<?php
class Foo {
    public function __toString(): string {
        return "Foo class";
    }
}

function test() {
    $f = new Foo();
    echo $f;
}

test();
