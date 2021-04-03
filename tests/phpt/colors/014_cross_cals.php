@kphp_should_fail
/Calling no-highload function from highload function \(f1\(\) call f5\(\)\)/
/  f1\(\) with following colors: \{highload\}/
/  f2\(\)/
/  f4\(\)/
/  f5\(\) with following colors: \{no-highload\}/
/Produced according to the following rule:/
/  "highload no-highload" => Calling no-highload function from highload function/
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
 * @kphp-color highload
 */
function f1() { f2(); }
function f2() { f3(); f4(); f6(); }
function f3() { echo 1; }
function f4() { f5(); }

/**
 * @kphp-color no-highload
 */
function f5() { echo 1; }

/**
 * @kphp-color highload
 */
function f6() { echo 1; }

f1();