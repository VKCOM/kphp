@kphp_should_fail k2_skip
/curl from api/
/api@api -> resum -> resum2 -> callurl@has-curl/
<?php

class KphpConfiguration {
  const FUNCTION_PALETTE = [
    [
      "api has-curl"    => "curl from api",
    ],
  ];
}

function init() {
    $ii = fork(api());
    wait($ii);
}

/** @kphp-color api */
function api() {
    sched_yield();
    $i = fork(resum());
    wait($i);
    return null;
}

function resum() {
    sched_yield();
    resum2();
    return null;
}

function init2() {
    $i = fork(resum2());
    wait($i);
}

function resum2() {
    sched_yield();
    callurl();
    return null;
}

/** @kphp-color has-curl */
function callurl() { sched_yield(); 1; }


init();
init2();
