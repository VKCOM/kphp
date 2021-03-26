@kphp_should_fail
/Calling function working with the database in the server side rendering function \(functionWithSsrAndMessageModuleAndHighload\(\) call functionWithDbAccessWithoutAllow\(\)\)/
/  functionWithSsrAndMessageModuleAndHighload\(\) with following colors\: \{ssr, message\-module, highload\}/
/  functionWithMessageModule\(\) with following colors\: \{message\-module\}/
/  functionWithDbAccessWithoutAllow\(\) with following colors\: \{has\-db\-access\}/
/Produced according to the following rule:/
/   "ssr has-db-access" => Calling function working with the database in the server side rendering function/
/Calling no\-highload function from highload function \(functionWithSsrAndMessageModuleAndHighload\(\) call functionWithAllowDbAccessAndNoHighload\(\)\)/
/  functionWithSsrAndMessageModuleAndHighload\(\) with following colors\: \{ssr, message\-module, highload\}/
/  functionWithMessageModule\(\) with following colors\: \{message\-module\}/
/  functionWithAllowDbAccessAndNoHighload\(\) with following colors\: \{has\-db\-access, ssr\-allow\-db, no\-highload\}/
/Produced according to the following rule:/
/   "highload no-highload" => Calling no-highload function from highload function/
/Calling function marked as internal outside of functions with the color message\-module \(dangerZoneCallingMessageInternals\(\) call messageInternals\(\)\)/
/  main\(\)/
/  dangerZoneCallingMessageInternals\(\) with following colors\: \{danger\-zone\}/
/  messageInternals\(\) with following colors\: \{message-internals}/
/Produced according to the following rule:/
/   "message-internals" => Calling function marked as internal outside of functions with the color message-module/
/Calling function without color danger\-zone in a function with color danger\-zone \(dangerZoneCallingMessageInternals\(\) call messageInternals\(\)\)/
/  dangerZoneCallingMessageInternals\(\) with following colors: \{danger\-zone\}/
/  messageInternals\(\) with following colors\: \{message-internals\}/
/Produced according to the following rule:/
/   "danger-zone *" => Calling function without color danger-zone in a function with color danger-zone/
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
      "api has-curl api-allow-curl"       => 1,
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
 * @kphp-color has-db-access ssr-allow-db
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