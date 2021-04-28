@kphp_should_fail
/dont allow rgb nesting/
/r@red -> r2 -> g@green -> g2 -> ssr1 -> ssr2 -> b@blue/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
      "ssr"    => 1,
      "api"    => 1,
    ],
    [
        'red green blue' => 'dont allow rgb nesting',
    ]
  ];
}

function init() { r(); }
init();

/**
 * @kphp-color red
 * @kphp-color api
 */
function r() { r2(); }
/** @kphp-color ssr */
function r2() { g(); b(); }

/** @kphp-color green */
function g() { g2(); }
function g2() { ssr1(); }

/**
  * @kphp-color blue
  */
function b() { b2(); }
function b2() { [1]; }

/** @kphp-color ssr */
function ssr1() { [1]; ssr2(); }
/** @kphp-color ssr */
function ssr2() { [1]; b(); }
