<?php
const eps = 1e-5;

function ensure($x)
{
    if (!$x) {
        header('HTTP/1.1 500 KML assertion failed', true, 500);
        die(1);
    }
}

function main()
{
    $inputs = array(
        array("height" => 1, "weight" => 1, "useless_feature" => 0.1),
        array("height" => 2, "weight" => 2, "useless_feature" => 0.12),
        array("height" => 100, "weight" => 100, "useless_feature" => 0.123),
        array("height" => 4, "weight" => -10, "useless_feature" => 0.1234),
        array("height" => 5, "weight" => -555, "useless_feature" => 0.12345),
    );
    $expected = array(
        0.745589,
        0.437018,
        0.437018,
        0.156840,
        0.156840,
    );

    $ans = kml_xgboost_predict_matrix("xgb_tiny_ht_remap_str_keys_to_fvalue", $inputs);

    $length = count($expected);
    ensure($length == count($ans));
    for ($i = 0; $i < $length; $i++) {
        ensure(abs($expected[$i] - $ans[$i]) < eps);
    }
}
main();
