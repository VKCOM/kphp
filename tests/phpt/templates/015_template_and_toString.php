@ok
<?php

class A {
    function __toString() { return __CLASS__; }
}
class B {
    function __toString() { return __CLASS__; }
}

/** @kphp-template $i */
function myTpl($i) {
    echo $i, "\n";
}

myTpl(5);
myTpl('d');
myTpl(new A);
myTpl(new B);

