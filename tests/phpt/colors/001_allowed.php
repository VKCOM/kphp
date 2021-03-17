@ok
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    "highload no-highload"              => "Calling no-highload function from highload function",
    "ssr has-db-access"                 => "Calling function working with the database in the server side rendering function",
    "ssr has-db-access ssr-allow-db"    => 1,
    "api has-curl"                      => "Calling curl function from API functions",
    "api api-callback has-curl"         => 1,
    "api has-curl api-allow-curl"       => 1,
    "message-internals"                 => "Calling function marked as internal outside of functions with the color message-module",
    "message-module message-internals"  => 1,
    "danger-zone *"                     => "Calling function without color danger-zone in a function with color danger-zone",
    "danger-zone danger-zone"           => 1,
  ];
}

/**
 * @kphp-color ssr
 */
function someSsr() {
    hasDbAccess();
}

/**
 * @kphp-color has-db-access ssr-allow-db
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
 * @kphp-color has-curl api-allow-curl
 */
function hasCurl() {
    echo 1;
}

api();


/**
 * @kphp-color api
 */
function apiCallApiCallback() {
    apiCallback();
}

/**
 * @kphp-color api-callback
 */
function apiCallback() {
    hasCurl();
}

apiCallApiCallback();


/**
 * @kphp-color api api-callback
 */
function apiWithApiCallback() {
    hasCurl();
}

apiWithApiCallback();


/**
 * @kphp-color message-internals
 */
function messageInternals() {
    echo 1;
}

/**
 * @kphp-color message-module
 */
function messageModule() {
    messageInternals();
}

messageModule();
