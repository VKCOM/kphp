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
        array(1996,
            197),
        array(1968,
            37),
        array(2002,
            77),
        array(1948,
            59),
    );

    $cat_feats = array(
        array("winter"),
        array("winter"),
        array("summer"),
        array("summer"),
    );

    $expected = array(
        array(-0.4315705250918395,
            -0.07602514583990287,
            0.5075956709317426),
        array(-0.7547556359399779,
            -0.9511000859169567,
            1.7058557218569348),
        array(-0.1531870128758389,
            0.3682398885336728,
            -0.21505287565783404),
        array(-0.04081236490074001,
            -0.7956756034401067,
            0.8364879683408469),
    );

    $ans = kml_catboost_predict_multi("catboost_tiny_vectors_float_and_categorial_multi", $float_feats, $cat_feats);

//     $length = count($expected);
//     ensure($length == count($ans));

//     $class_cnt = count($expected[0]);
//     for ($i = 0; $i < $length; $i++) {
// //         ensure($class_cnt == count($ans[$i]));
//         for ($j = 0; $j < $class_cnt; $j++) {
// //             ensure(abs($expected[$i][$j] - $ans[$i][$j]) < eps);
//         }
//     }
}
main();
