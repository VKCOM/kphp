@kphp_should_fail
/construct not implemented/
<?php

function inverse($x) {
    if (!$x) {
        throw new Exception('Division by zero.');
    }
    return 1/$x;
}

try {
    echo inverse(5);
} catch (Exception $e) {
    // ...
} finally {
    echo "without throw";
}
