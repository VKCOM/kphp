@kphp_should_fail
/Calling curl function from API functions \(api\(\) call hasCurl\(\)\)/
/  api\(\) with following colors\: \{api\}/
/  hasCurl\(\) with following colors\: \{has-curl\}/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    "api has-curl"                => "Calling curl function from API functions",
    "api api-callback has-curl"   => 1,
    "api has-curl api-allow-curl" => 1,
  ];
}

/**
 * @kphp-color api
 */
function api() {
    hasCurl();
}

/**
 * @kphp-color has-curl
 */
function hasCurl() {
    echo 1;
}

api();
