@kphp_should_fail
\throw expression isn't supported\

<?php
try {
    $val = throw new Exception();
} catch (Exception $err) {
    // ...
}
