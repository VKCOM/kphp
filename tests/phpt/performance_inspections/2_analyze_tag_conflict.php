@kphp_should_fail
/@kphp-analyze-performance conflict, one caller enables 'implicit-array-cast' while other disables it\
<?php

function ab() { var_dump("ab"); }
function a2() { ab(); }
function b2() { ab(); }

/**
 *  @kphp-analyze-performance array-merge-into implicit-array-cast
 */
function a1() { a2(); }

/**
 *  @kphp-analyze-performance all !implicit-array-cast
 */
function b1() { b2(); }

a1();
b1();
