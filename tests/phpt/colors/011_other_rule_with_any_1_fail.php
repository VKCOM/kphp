@kphp_should_fail
/error 1 \(dangerZone\(\) call hasCurl\(\)\)/
/  dangerZone\(\)/
/  hasCurl\(\)/
/Produced according to the following rule:/
/  "\* \*" => error 1/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "* *" => "error 1",
        "* highload" => 1,
    ],
  ];
}

function dangerZone() {
    highload(); // ok
    hasCurl(); // error
}

/**
 * @kphp-color highload
 */
function highload() {
    echo 1;
}

function hasCurl() {
    echo 1;
}

dangerZone();
