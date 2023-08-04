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
    echo inverse(0);
} catch (Exception $e) {
    // ...
} finally {
    echo "with throw";
}
