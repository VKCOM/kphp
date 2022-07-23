@kphp_should_fail
/pass int to argument \$instance of instance_cast/
/but it's declared as @param object/
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

f();
