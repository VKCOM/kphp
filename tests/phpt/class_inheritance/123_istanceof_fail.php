@kphp_should_fail
/left operand of 'instanceof' should be an instance of class, but passed type 'int'/
/left operand of 'instanceof' should be an instance of class, but passed type 'int'/
<?php

interface IBase {}
class Base implements IBase {}
class ExtBase extends Base {}

function f() {
    $b = 100;

    if ($b instanceof ExtBase) {
        echo 1;
    } else if ($b instanceof Base) {
        echo 2;
    }
}

f(new Base);
