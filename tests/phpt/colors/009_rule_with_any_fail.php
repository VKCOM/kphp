@kphp_should_fail
/Calling function without color danger\-zone in a function with color danger\-zone \(dangerZone\(\) call functionWithSomeColors\(\)\)/
/  dangerZone\(\) with following colors\: \{danger\-zone\}/
/  functionWithSomeColors\(\) with following colors: \{ssr\}/
/Produced according to the following rule:/
/  "danger-zone \*" => Calling function without color danger-zone in a function with color danger-zone/
/Calling function without color danger\-zone in a function with color danger\-zone \(dangerZone\(\) call functionWithoutColors\(\)\)/
/  dangerZone\(\) with following colors\: \{danger\-zone\}/
/  functionWithoutColors\(\)/
/Produced according to the following rule:/
/  "danger-zone \*" => Calling function without color danger-zone in a function with color danger-zone/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "danger-zone *"           => "Calling function without color danger-zone in a function with color danger-zone",
        "danger-zone danger-zone" => 1,
    ],
    [
        "ssr has-db-access"                 => "Calling function working with the database in the server side rendering function",
        "ssr has-db-access ssr-allow-db"    => 1,
    ],
  ];
}

/**
 * @kphp-color danger-zone
 */
function dangerZone() {
    functionWithSomeColors();
    functionWithoutColors();
    echo 1;
}

/**
 * @kphp-color ssr
 */
function functionWithSomeColors() {
    echo 1;
}

function functionWithoutColors() {
    echo 1;
}

functionWithSomeColors();
dangerZone();
