@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_geoToH3() {
  assert_int_eq3(\UberH3::geoToH3(0, 0, 16), 0);
  assert_int_eq3(\UberH3::geoToH3(0, 0, -1), 0);

  assert_int_eq3(\UberH3::geoToH3(0, 0, 0), 578536630256664575);
  assert_int_eq3(\UberH3::geoToH3(0, 0, 1), 583031433791012863);
  assert_int_eq3(\UberH3::geoToH3(0, 0, 2), 587531734883500031);
  assert_int_eq3(\UberH3::geoToH3(-0.45969564234129, 0.53416786298236, 2), 587531734883500031);
  assert_int_eq3(\UberH3::geoToH3(0, 0, 15), 646078419604526808);

  assert_int_eq3(\UberH3::geoToH3(30, 50, 11), 626785114471653375);
  assert_int_eq3(\UberH3::geoToH3(30.000045196706, 50.00011997391, 11), 626785114471653375);
  assert_int_eq3(\UberH3::geoToH3(70, 120, 7), 608083127927046143);
  assert_int_eq3(\UberH3::geoToH3(90, 90, 6), 603537747495354367);
  assert_int_eq3(\UberH3::geoToH3(70, 120, 3), 590068789245116415);
}

/**
 * @param $lhs tuple(float, float)
 * @param $rhs tuple(float, float)
 */
function assert_lat_lon($lhs, $rhs) {
  assert_near($lhs[0], $rhs[0]);
  assert_near($lhs[1], $rhs[1]);
}

function test_h3ToGeo() {
  assert_lat_lon(\UberH3::h3ToGeo(0), tuple(79.242398509759, 38.02340700797));
  assert_lat_lon(\UberH3::h3ToGeo(1), tuple(79.242398509759, 38.02340700797));
  assert_lat_lon(\UberH3::h3ToGeo(2), tuple(79.242398509759, 38.02340700797));

  assert_lat_lon(\UberH3::h3ToGeo(578536630256664575), tuple(2.3008821116267, -5.2453902967773));
  assert_lat_lon(\UberH3::h3ToGeo(587531734883500031), tuple(-0.45969564234129, 0.53416786298236));
  assert_lat_lon(\UberH3::h3ToGeo(626785114471653375), tuple(30.000045196706, 50.00011997391));
}

function test_h3ToGeoBoundary() {
  $boundary = \UberH3::h3ToGeoBoundary(0);
  assert_int_eq3(count($boundary), 6);
  assert_lat_lon($boundary[0], tuple(68.92995788194, 31.831280499087));
  assert_lat_lon($boundary[1], tuple(69.393596489918, 62.34534495651));
  assert_lat_lon($boundary[2], tuple(76.163042830191, 94.143090101848));
  assert_lat_lon($boundary[3], tuple(87.364695323196, 145.55819769134));
  assert_lat_lon($boundary[4], tuple(81.271371790205, -34.758417980285));
  assert_lat_lon($boundary[5], tuple(73.310223685444, 0.32561035194326));

  $boundary = \UberH3::h3ToGeoBoundary(587531734883500031);
  assert_int_eq3(count($boundary), 6);
  assert_lat_lon($boundary[0], tuple(-1.6678524470433, 1.2456980510073));
  assert_lat_lon($boundary[1], tuple(-0.25487352552886, 1.9211969045936));
  assert_lat_lon($boundary[2], tuple(0.93616376459837, 1.2017635601568));
  assert_lat_lon($boundary[3], tuple(0.71979821137039, -0.16020343694525));
  assert_lat_lon($boundary[4], tuple(-0.65938768173489, -0.81995087881793));
  assert_lat_lon($boundary[5], tuple(-1.8554926185477, -0.13368891259165));
}


test_geoToH3();
test_h3ToGeo();
test_h3ToGeoBoundary();
