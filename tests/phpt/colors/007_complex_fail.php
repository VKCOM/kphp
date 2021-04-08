@kphp_should_fail
/Calling function marked as internal outside of functions with the color message-module/
/ -> dangerZoneCallingMessageInternals -> messageInternals@message-internals/
/Calling no-highload function from highload function/
/functionWithSsrAndMessageModuleAndHighload@highload -> functionWithMessageModule -> functionWithAllowDbAccessAndNoHighload@no-highload/
/Calling function working with the database in the server side rendering function/
/functionWithSsrAndMessageModuleAndHighload@ssr -> functionWithMessageModule -> functionWithAllowDbAccessAndNoHighload@has-db-access/
/functionWithSsrAndMessageModuleAndHighload@ssr -> functionWithMessageModule -> functionWithDbAccessWithoutAllow@has-db-access/
<?php

define('DEFINED_ERR_TXT', "Calling function working with the database in the server side rendering function");

class E1 {
    const E1_TXT = "Calling no-highload function from highload function";
    const E2_TXT = self::E1_TXT;
}

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
      "highload no-highload"              => E1::E2_TXT,
    ],
    [
      "ssr has-db-access"                 => DEFINED_ERR_TXT,
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
 * @kphp-color message-module
 * @kphp-color highload
 */
function functionWithSsrAndMessageModuleAndHighload() {
    functionWithMessageModule();
}

/**
 * @kphp-color message-module
 */
function functionWithMessageModule() {
    functionWithAllowDbAccessAndNoHighload(); // error, call no-highload in highload
    functionWithDbAccessWithoutAllow(); // error, db is not allowed
}

/**
 * @kphp-color has-db-access
 * @kphp-color ssr-allow-db
 * @kphp-color no-highload
 */
function functionWithAllowDbAccessAndNoHighload() {
    echo 1;
}

/**
 * @kphp-color has-db-access
 */
function functionWithDbAccessWithoutAllow() {
    echo 1;
}


/**
 * @kphp-color danger-zone
 */
function dangerZoneCallingMessageInternals() {
    messageInternals();
}

/**
 * @kphp-color message-internals
 */
function messageInternals() {
    echo 1;
}


function main() {
    dangerZoneCallingMessageInternals();
    functionWithSsrAndMessageModuleAndHighload();
}

main();
