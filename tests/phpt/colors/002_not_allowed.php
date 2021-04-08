@kphp_should_fail
/Calling function working with the database in the server side rendering function/
/someSsr@ssr -> hasDbAccess@has-db-access/
/Calling curl function from API functions/
/api@api -> hasCurl@has-curl/
<?php


class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
      "highload no-highload"              => "Calling no-highload function from highload function",
    ],
    [
      "ssr has-db-access"                 => "Calling function working with the database in the server side rendering function",
      "ssr ssr-allow-db has-db-access"    => 1,
    ],
    [
      "api has-curl"                      => "Calling curl function from API functions",
      "api api-callback has-curl"         => 1,
      "api api-allow-curl has-curl"       => 1,
    ],
    [
      "message-internals"                 => "Calling function marked as internal outside of functions with the color message-module",
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
