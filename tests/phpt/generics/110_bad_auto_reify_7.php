@kphp_should_fail
/mapOne\(\) does not return an instance or it can't be detected/
/Couldn't reify generic <TOut> for call/
<?php

class B {
    function fb() { echo "B\n"; }
}

/**
 * @kphp-generic TIn, TOut
 * @param TIn $in
 * @param callable(TIn):TOut $cb
 * @return TOut
 */
function mapOne($in, $cb) {
    $out = $cb($in);
    return $out;
}

mapOne(new B, function($_) {
    return;
})->fb();
