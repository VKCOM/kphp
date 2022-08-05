@kphp_should_fail
/Parse file/
/tuple\(int/
/Failed to parse @var inside invalidPhpdoc/
<?php

function invalidPhpdoc() {
    /** @var tuple(int */
    $i = 0;
}

invalidPhpdoc();
