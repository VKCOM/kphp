@kphp_should_fail
/Redeclaration/
<?php

trait Tr {
    public $x = 20;
}

class A {
    use Tr;
    public $x = 20;
}
