@kphp_should_fail
/KphpConfiguration\:\:FUNCTION_PALETTE in the palette, the result of the rule must be either the number 1 or a string with an error/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    "no-highload highload"              => "Calling no-highload function from highload function",
    "ssr has-db-access"                 => "Calling function working with the database in the server side rendering function",
    "ssr ssr-allow-db has-db-access"    => 1,
    "api has-curl"                      => "Calling curl function from API functions",
    "api api-callback has-curl"         => 1,
    "api api-allow-curl has-curl"       => 1,
    "message-internals"                 => "Calling function marked as internal outside of functions with the color message-module",
    "message-module message-internals"  => 1,
    "danger-zone *"                     => "Calling function without color danger-zone in a function with color danger-zone",
    "danger-zone danger-zone"           => 1,

    "api has-curl" => 2, // error value
  ];
}
