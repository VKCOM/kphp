@kphp_should_fail
/dont call curl from api/
/f3@api -> f10 -> f11 -> f12 -> f1 -> f2@has-curl/
<?php

class KphpConfiguration {
    const FUNCTION_PALETTE = [
        [
            'api has-curl' => 'dont call curl from api',
            'api has-curl please' => 1,
        ]
    ];
}

function f1() { f2(); f3(); }
/**
 * @kphp-color has-curl
 */
function f2() { f10(); }
/** @kphp-color api */
function f3() { f10(); }
function f10() { f11(); f100(); }
function f11() { [1]; f12(); f13(); }
function f12() { [2]; f1(); }
function f13() { [3]; }
function f100() { [100]; f101(); }
function f101() { [101]; f102(); }
function f102() { [101]; f101(); }
f1();
