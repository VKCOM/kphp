@kphp_should_fail
/class A must be abstract: method IWithStatic::foo is not overridden/
<?php

interface IWithStatic {
    public static function foo($x);
}

class A implements IWithStatic {
}

