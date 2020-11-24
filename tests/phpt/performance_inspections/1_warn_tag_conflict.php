@kphp_should_fail
/@kphp-warn-performance conflict, one caller enables 'implicit-array-cast' while other disables it\nEnabled by: a1 -> a2 -> ab\nDisabled by: b1 -> b2 -> ab\
<?php

function ab() { var_dump("ab"); }
function a2() { ab(); }
function b2() { ab(); }

/**
 *  @kphp-warn-performance array-merge-into implicit-array-cast
 */
function a1() { a2(); }

/**
 *  @kphp-warn-performance all !implicit-array-cast
 */
function b1() { b2(); }

a1();
b1();
