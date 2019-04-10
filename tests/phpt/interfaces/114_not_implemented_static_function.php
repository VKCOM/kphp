@kphp_should_fail
<?php

interface IWithStatic {
    public static function foo($x);
}

class A implements IWithStatic {
}

