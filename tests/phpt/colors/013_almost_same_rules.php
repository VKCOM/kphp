@kphp_should_fail
/Calling curl function from API functions \(api\(\) call hasCurl\(\)\)/
/  api\(\) with following colors: \{api\}/
/  api2\(\) with following colors: \{api2\}/
/  hasCurl\(\) with following colors: \{has-curl\}/
/Produced according to the following rule:/
/  "api has-curl" => Calling curl function from API functions/
/Calling curl function from API2 functions \(api2\(\) call hasCurl\(\)\)/
/  api2\(\) with following colors: \{api2\}/
/  hasCurl\(\) with following colors: \{has-curl\}/
/Produced according to the following rule:/
/  "api2 has-curl" => Calling curl function from API2 functions/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "api has-curl"                      => "Calling curl function from API functions",
        "api api-callback has-curl"         => 1,
        "api api-allow-curl has-curl"       => 1,
    ],
    [
        "api2 has-curl"                      => "Calling curl function from API2 functions",
        "api2 api-callback has-curl"         => 1,
        "api2 has-curl api-allow-curl"       => 1,
    ],
  ];
}

/**
 * @kphp-color api
 */
function api() {
    api2();
    hasCurl();
}

/**
 * @kphp-color api2
 */
function api2() {
    hasCurl();
}

/**
 * @kphp-color has-curl
 */
function hasCurl() {
    echo 1;
}

api();
