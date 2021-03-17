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
 * @kphp-color message-module
 */
function functionWithSsrAndMessageModule() {
    functionWithMessageModule();
}

/**
 * @kphp-color message-module
 */
function functionWithMessageModule() {
    functionWithAllowDbAccessAndMessageInternals(); // ok, has allow color
}

/**
 * @kphp-color has-db-access ssr-allow-db
 * @kphp-color message-internals
 */
function functionWithAllowDbAccessAndMessageInternals() {
    functionWithMessageInternals();
}

/**
 * @kphp-color message-internals
 */
function functionWithMessageInternals() {
    echo 1;
}

functionWithSsrAndMessageModule();
