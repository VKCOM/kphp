@kphp_should_fail
/dont call curl from api/
/@api -> /
/ -> f4@has-curl/
<?php
// a strange error pattern is because it's not unique and can vary from launch to launch

class KphpConfiguration {
    const FUNCTION_PALETTE = [
        [
            'api has-curl' => 'dont call curl from api',
            'api has-curl please' => 1,
        ]
    ];
}


function cuf($name) {
    if(0) api_1();
    if(0) api_2();
    api_3(); api_4(); api_5(); api_6();
    f1(); f2(); f3(); f4(); f5(); f6();
}

/** @kphp-color api */
function api_1() { f1(); f2(); f3(); api_2(); }
function f1() { cuf('api_1'); }

/** @kphp-color api */
function api_2() { f2(); f1(); }
function f2() { cuf('api_2'); }

/** @kphp-color api */
function api_3() { f3(); f4(); f1();  }
function f3() { cuf('api_3'); }

/** @kphp-color api */
function api_4() { f4();  }
/** @kphp-color has-curl */
function f4() { cuf('api_3'); }

/** @kphp-color api */
function api_5() { f5();  }
function f5() { cuf('api_3'); }

/** @kphp-color api */
function api_6() { f6();  }
function f6() { cuf('api_3'); }

cuf('api_1');

