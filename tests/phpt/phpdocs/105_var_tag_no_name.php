@kphp_should_fail
/int\|false/
/@var with empty var name/
<?php

function demo() {
    /** @var int|false */
    echo 1;
}

demo();
