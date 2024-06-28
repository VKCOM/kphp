@kphp_should_fail
\throw expression isn't supported\

<?php
try {
    $falsableValue = false;
    $value = $falsableValue ?: throw new Exception("error");
} catch (Exception $err) {
    // ...
}
