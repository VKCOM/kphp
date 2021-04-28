@kphp_should_fail
/Calling curl function from API functions/
/api@api -> hasCurl@has-curl/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "api has-curl"                => "Calling curl function from API functions",
        "api api-callback has-curl"   => 1,
        "api api-allow-curl has-curl" => 1,
    ],
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
