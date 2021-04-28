@kphp_should_fail
/Calling curl function from API functions/
/api2@api -> common -> callurl@has-curl/
<?php

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
function allow2() { api2(); }
/** @kphp-color api */
function api2() { common(); }

function common() { callurl(); }
/** @kphp-color has-curl */
function callurl() { [1]; }

allow2();
api1();
