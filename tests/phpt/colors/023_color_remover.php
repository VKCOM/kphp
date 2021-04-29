@ok
<?php

class KphpConfiguration {
    const FUNCTION_PALETTE = [
        [
            'highload no-highload' => 'error text',
        ]
    ];
}

/** @kphp-color remover */
function callAct() {
    if(0) f1();
    if(0) f2();
    if(0) f3();
    if(0) f4();
}


/** @kphp-color highload */
function f1() { g1(); }
function g1() { callAct(); }

/** @kphp-color highload */
function f2() { g2(); }
function g2() { callAct(); }

function f3() { g3(); }
/** @kphp-color no-highload */
function g3() { callAct(); }

function f4() { g4(); }
/** @kphp-color no-highload */
function g4() { callAct(); }

callAct();
