@kphp_should_fail
/Calling function working with the database in the server side rendering function \(someSsr\(\) call hasDbAccess\(\)\)/
/  someSsr\(\) with following colors\: \{ssr\}/
/  hasDbAccess\(\) with following colors\: \{has-db-access\}/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    "ssr has-db-access"              => "Calling function working with the database in the server side rendering function",
    "ssr has-db-access ssr-allow-db" => 1,
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
