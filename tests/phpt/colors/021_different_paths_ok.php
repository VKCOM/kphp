@ok
<?php
require_once 'kphp_tester_include.php';

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
        "api has-curl"                      => "Calling curl function from API functions",
        "api api-callback has-curl"         => 1,
        "api api-allow-curl has-curl"       => 1,
    ],
  ];
}

/** @kphp-color api */
function api1() { allow1(); }
/** @kphp-color api-allow-curl */
function allow1() { common(); }

/** @kphp-color api-allow-curl */
function allow2() { common(); }
/** @kphp-color api */
function api2() { allow2(); }

function common() { callurl(); }
/** @kphp-color has-curl */
function callurl() { [1]; }

api2();
api1();
