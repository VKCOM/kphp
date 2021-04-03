@ok
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
function f1() { f2(); }
/**
 * @kphp-color ssr-allow-db
 */
function f2() { f3(); }
/**
 * @kphp-color has-db-access
 */
function f3() { f4(); }

function f4() { echo 1; }
function f5() { echo 1; }

/**
 * @kphp-color ssr
 */
function f6() { f7(); }
/**
 * @kphp-color has-db-access
 */
function f7() { echo 1; /* not an error, since above in the f2() function there was a color allowing this */ }

f1();