@ok
<?php

require_once 'kphp_tester_include.php';

function test_h3GetResolution() {
  assert_int_eq3(\UberH3::h3GetResolution(0), 0);
  assert_int_eq3(\UberH3::h3GetResolution(1), 0);

  assert_int_eq3(\UberH3::h3GetResolution(608083127927046143), 7);
  assert_int_eq3(\UberH3::h3GetResolution(587531734883500031), 2);

  assert_int_eq3(\UberH3::h3GetResolution(\UberH3::geoToH3(0, 11, 0)), 0);
  assert_int_eq3(\UberH3::h3GetResolution(\UberH3::geoToH3(70, 12, 1)), 1);
  assert_int_eq3(\UberH3::h3GetResolution(\UberH3::geoToH3(15, 12, 2)), 2);
  assert_int_eq3(\UberH3::h3GetResolution(\UberH3::geoToH3(70, 120, 3)), 3);
  assert_int_eq3(\UberH3::h3GetResolution(\UberH3::geoToH3(32, 41, 8)), 8);
  assert_int_eq3(\UberH3::h3GetResolution(\UberH3::geoToH3(4, 4, 15)), 15);
}

function test_h3GetBaseCell() {
  assert_int_eq3(\UberH3::h3GetBaseCell(0), 0);
  assert_int_eq3(\UberH3::h3GetBaseCell(1), 0);

  assert_int_eq3(\UberH3::h3GetBaseCell(608083127927046143), 2);
  assert_int_eq3(\UberH3::h3GetBaseCell(587531734883500031), 58);

  assert_int_eq3(\UberH3::h3GetBaseCell(\UberH3::geoToH3(0, 11, 0)), 65);
  assert_int_eq3(\UberH3::h3GetBaseCell(\UberH3::geoToH3(70, 12, 1)), 4);
  assert_int_eq3(\UberH3::h3GetBaseCell(\UberH3::geoToH3(15, 12, 2)), 44);
  assert_int_eq3(\UberH3::h3GetBaseCell(\UberH3::geoToH3(70, 120, 3)), 2);
  assert_int_eq3(\UberH3::h3GetBaseCell(\UberH3::geoToH3(32, 41, 8)), 22);
  assert_int_eq3(\UberH3::h3GetBaseCell(\UberH3::geoToH3(4, 4, 15)), 65);
}

function test_stringToH3() {
  assert_int_eq3(\UberH3::stringToH3("0"), 0);
  assert_int_eq3(\UberH3::stringToH3("1"), 1);

  assert_int_eq3(\UberH3::stringToH3("870586211ffffff"), 608083127927046143);
  assert_int_eq3(\UberH3::stringToH3("82754ffffffffff"), 587531734883500031);
}

function test_h3ToString() {
  assert_str_eq3(\UberH3::h3ToString(0), "0");
  assert_str_eq3(\UberH3::h3ToString(1), "1");

  assert_str_eq3(\UberH3::h3ToString(608083127927046143), "870586211ffffff");
  assert_str_eq3(\UberH3::h3ToString(587531734883500031), "82754ffffffffff");
}

function test_h3IsValid() {
  assert_false(\UberH3::h3IsValid(0));
  assert_false(\UberH3::h3IsValid(1));

  assert_true(\UberH3::h3IsValid(578536630256664575));
  assert_true(\UberH3::h3IsValid(587531734883500031));

  assert_true(\UberH3::h3IsValid(\UberH3::geoToH3(70, 120, 3)));
  assert_true(\UberH3::h3IsValid(\UberH3::geoToH3(-10, 14, 7)));
}

function test_h3IsResClassIII() {
  assert_false(\UberH3::h3IsResClassIII(0));
  assert_false(\UberH3::h3IsResClassIII(1));

  assert_true(\UberH3::h3IsResClassIII(608083127927046143));
  assert_false(\UberH3::h3IsResClassIII(603537747495354367));

  assert_true(\UberH3::h3IsResClassIII(\UberH3::geoToH3(0, 0, 3)));
  assert_false(\UberH3::h3IsResClassIII(\UberH3::geoToH3(0, 0, 4)));
}

function test_h3IsPentagon() {
  assert_false(\UberH3::h3IsPentagon(0));
  assert_false(\UberH3::h3IsPentagon(1));

  assert_false(\UberH3::h3IsPentagon(608083127927046143));
  assert_false(\UberH3::h3IsPentagon(603537747495354367));

  assert_false(\UberH3::h3IsPentagon(\UberH3::geoToH3(0, 0, 3)));
  assert_false(\UberH3::h3IsPentagon(\UberH3::geoToH3(0, 0, 4)));
}

function test_h3GetFaces() {
  assert_array_int_eq3(\UberH3::h3GetFaces(608083127927046143), [1, -1]);
  assert_array_int_eq3(\UberH3::h3GetFaces(\UberH3::geoToH3(0, 0, 3)), [9, -1]);
}

function test_maxFaceCount() {
  assert_int_eq3(\UberH3::maxFaceCount(608083127927046143), 2);
  assert_int_eq3(\UberH3::maxFaceCount(\UberH3::geoToH3(0, 0, 3)), 2);
}


test_h3GetResolution();
test_h3GetBaseCell();
test_stringToH3();
test_h3ToString();
test_h3IsValid();
test_h3IsResClassIII();
test_h3IsPentagon();
test_h3GetFaces();
test_maxFaceCount();
