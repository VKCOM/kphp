@ok
<?php

/**@kphp-infer*/
class A {
    public $x = "asdf";
}

/**@kphp-infer*/
class B extends A {
    public $y = 10;
}

$b = new A();
clone $b;

