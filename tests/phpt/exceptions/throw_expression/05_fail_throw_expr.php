@kphp_should_fail
\throw expression isn't supported\

<?php
try {
    $condition = true;
    if ($condition || throw new Exception()) {
        // ...
    }
} catch (Exception $err) {
    echo $err->getMessage();
}
