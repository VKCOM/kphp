@kphp_should_fail
\throw expression isn't supported\

<?php
try {
    $array = [];
    $value = !empty($array) ? $array : throw new Exception();
} catch (Exception $err) {
    // ...
}
