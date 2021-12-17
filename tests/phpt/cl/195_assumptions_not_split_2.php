@kphp_should_fail
KPHP_SHOW_ALL_TYPE_ERRORS=1
/assign A to \$a7, modifying a function argument/
<?php

class A { function fa() {} }

function demo7(string $a7) {
    echo $a7;
    $a7 = new A;
    $a7->fa();
}

demo7('');

