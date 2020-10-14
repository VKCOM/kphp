@ok
<?php

class A {
    public $x = "asdf";
}

class B extends A {
    public $y = 10;
}

$b = new A();
clone $b;

