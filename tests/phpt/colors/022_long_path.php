@kphp_should_fail
/function fHigh\(\)/
/fHigh@highload -> longDisallowedPath -> l1 -> l2 -> l3 -> l4 -> fLow@no-highload/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
      "highload no-highload"          => 'Calling no-highload function from highload function',
      'highload allow-hh no-highload' => 1,
    ],
];
}

/** @kphp-color highload */
function fHigh() {
     shortAllowedPath();
     longDisallowedPath();
}
fHigh();

/** @kphp-color no-highload */
function fLow() { [1]; }

function shortAllowedPath() {
    s1();
}

/** @kphp-color allow-hh */
function s1() { fLow(); }

function longDisallowedPath() {
    l1();
}

function l1() { l2(); }
function l2() { l3(); }
function l3() { l4(); }
function l4() { fLow(); }

