<?php

/** @return shape(features_map:float[], ans:float[])[] */
function getInput() {
return [
   shape([
    "features_map" => [
      "year" => 1996,
      "count" => 197,
      "season" => 1
    ],
    "ans" => [
      -0.4315705250918395,
      -0.07602514583990287,
      0.5075956709317426
    ]
  ]),
  shape([
    "features_map" => [
      "year" => 1968,
      "count" => 37,
      "season" => 1
    ],
    "ans" => [
      -0.7547556359399779,
      -0.9511000859169567,
      1.7058557218569348
    ]
  ]),
  shape([
    "features_map" => [
      "year" => 2002,
      "count" => 77,
      "season" => 0
    ],
    "ans" => [
      -0.1531870128758389,
      0.3682398885336728,
      -0.21505287565783404
    ]
  ]),
  shape([
    "features_map" => [
      "year" => 1948,
      "count" => 59,
      "season" => 0
    ],
    "ans" => [
      -0.04081236490074001,
      -0.7956756034401067,
      0.8364879683408469
    ]
  ])
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
        $predictions = kml_catboost_predict_ht_multi("catboost_multiclass_tutorial_ht_catnum", $row['features_map']);
        ensure(count($predictions) == count($row['ans']));
        for ($i = 0; $i < count($predictions); $i++) {
            ensure(abs($predictions[$i] - $row['ans'][$i]) < eps);
        }
    }
}
main();
