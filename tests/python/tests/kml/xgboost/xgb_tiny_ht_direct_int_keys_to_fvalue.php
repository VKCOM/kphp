<?php
const eps = 1e-5;

function ensure($x)
{
    if (!$x) {
        die(1);
    }
}

function main()
{
    $inputs = array(
        array("0" => 1, "1" => 1),
        array("0" => 2, "1" => 2),
        array("0" => 100, "1" => 100),
        array("0" => 4, "1" => -10),
        array("0" => 5, "1" => -555),
    );
    $expected = array(
        0.745589,
        0.437018,
        0.437018,
        0.156840,
        0.156840,
    );

    $ans = kml_xgb_predict("xgb_tiny_ht_direct_int_keys_to_fvalue", $inputs);

    $length = count($expected);
    ensure($length == count($ans));
    for ($i = 0; $i < $length; $i++) {
        ensure(abs($expected[$i] - $ans[$i]) < eps);
    }
}
main();
