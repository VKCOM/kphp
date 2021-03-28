@kphp_should_fail
/Calling function working with the database in the server side rendering function \(someSsr\(\) call hasDbAccess\(\)\)/
/  someSsr\(\) with following colors\: \{ssr\}/
/  hasDbAccess\(\) with following colors\: \{has-db-access\}/
/Produced according to the following rule:/
/  "ssr has-db-access" => Calling function working with the database in the server side rendering function/
/Calling curl function from API functions \(api\(\) call hasCurl\(\)\)/
/  api\(\) with following colors\: \{api\}/
/  hasCurl\(\) with following colors\: \{has-curl\}/
/Produced according to the following rule:/
/  "api has-curl" => Calling curl function from API functions/
<?php


class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
      "highload no-highload"              => "Calling no-highload function from highload function",
    ],
    [
      "ssr has-db-access"                 => "Calling function working with the database in the server side rendering function",
      "ssr has-db-access ssr-allow-db"    => 1,
    ],
    [
      "api has-curl"                      => "Calling curl function from API functions",
      "api api-callback has-curl"         => 1,
      "api api-allow-curl has-curl"       => 1,
    ],
    [
      "* message-internals"                 => "Calling function marked as internal outside of functions with the color message-module",
      "message-module message-internals"  => 1,
    ],
    [
      "danger-zone *"                     => "Calling function without color danger-zone in a function with color danger-zone",
      "danger-zone danger-zone"           => 1,
    ],
  ];
}

/**
 * @kphp-color ssr
 */
function someSsr() {
    hasDbAccess();
}

/**
 * @kphp-color has-db-access
 */
function hasDbAccess() {
    echo 1;
}

someSsr();


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
