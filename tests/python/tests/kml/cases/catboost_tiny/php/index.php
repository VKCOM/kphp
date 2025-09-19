<?php

/** @return shape(float_features:float[], cat_features:string[], ans:float)[] */
function getInput() {
return [
    shape([
        "float_features" => [
            1
        ],
        "cat_features" => [
            "a"
        ],
        "ans" => 0.7363564860973357
    ]),
    shape([
        "float_features" => [
            2
        ],
        "cat_features" => [
            "a"
        ],
        "ans" => 1.4672203311538694
    ]),
    shape([
        "float_features" => [
            3
        ],
        "cat_features" => [
            "a"
        ],
        "ans" => -2.276542165549472
    ]),
    shape([
        "float_features" => [
            3
        ],
        "cat_features" => [
            "b"
        ],
        "ans" => -0.9489540606737137
    ]),
    shape([
        "float_features" => [
            5
        ],
        "cat_features" => [
            "b"
        ],
        "ans" => -0.9489540606737137
    ]),
];
}

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
    $input = getInput();
    foreach ($input as $row) {
        $prediction = kml_catboost_predict_vectors("catboost_tiny_1float_1hot_10trees", $row['float_features'], $row['cat_features']);
        ensure(abs($prediction - $row['ans']) < eps);
    }
}
main();
