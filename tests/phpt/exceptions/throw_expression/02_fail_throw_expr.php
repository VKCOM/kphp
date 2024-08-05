@kphp_should_fail
\throw expression isn't supported\

<?php
try {
    $value = $nullableValue ?? throw new Exception();
} catch (Exception $err) {
    // ...
}
