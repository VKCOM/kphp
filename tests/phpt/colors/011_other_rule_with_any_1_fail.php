@kphp_should_fail
/error 1 \(dangerZone\(\) call hasCurl\(\)\)/
/  dangerZone\(\) with following colors\: \{danger\-zone\}/
/  hasCurl\(\) with following colors\: \{has\-curl\}/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    "* *" => "error 1",
    "* highload" => 1,
  ];
}

/**
 * @kphp-color danger-zone
 */
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

/**
 * @kphp-color has-curl
 */
function hasCurl() {
    echo 1;
}

dangerZone();
