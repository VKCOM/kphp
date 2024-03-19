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
    $float_feats = array(
        array(1, 1),
        array(2, 8),
        array(4, 6),
        array(5, 0.5),
        array(10, -5),
        array(10, 5),
        array(10, 100),
    );

    $cat_feats = array(
        array("a"),
        array("a"),
        array("d"),
        array("d"),
        array("d"),
        array("d"),
        array("d"),
    );

    $expected = array(
        0.45550728872420354,
        -0.6373624799572901,
        -0.4773922417644849,
        2.381618389160235,
        2.6270874294276063,
        -5.5588669688490215,
        -5.5588669688490215,
    );

    $ans = kml_catboost_predict("catboost_tiny_vectors_float_and_categorial", $float_feats, $cat_feats);

    $length = count($expected);
    ensure($length == count($ans));
    for ($i = 0; $i < $length; $i++) {
        ensure(abs($expected[$i] - $ans[$i]) < eps);
    }
}
main();
