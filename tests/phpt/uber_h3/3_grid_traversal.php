@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_kRing() {
  assert_true(\UberH3::kRing(603537747495354367, -1) === false);

  assert_array_int_eq3(\UberH3::kRing(603537747495354367, 0), [603537747495354367]);
  assert_array_int_eq3(\UberH3::kRing(603537747495354367, 1), [
    603537747495354367,
    603537753803587583,
    603537753266716671,
    603537746958483455,
    603537746690047999,
    603537747226918911,
    603537751387668479
  ]);
}

function test_maxKringSize() {
  assert_int_eq3(\UberH3::maxKringSize(-1),0);

  assert_int_eq3(\UberH3::maxKringSize(0), 1);
  assert_int_eq3(\UberH3::maxKringSize(1), 7);
  assert_int_eq3(\UberH3::maxKringSize(5), 91);
}

function test_kRingDistances() {
  assert_true(\UberH3::kRingDistances(603537747495354367, -1) === false);

  $kring = \UberH3::kRingDistances(603537747495354367, 0);
  assert_int_eq3(count($kring), 1);
  assert_int_eq3($kring[0][0], 603537747495354367);
  assert_int_eq3($kring[0][1], 0);

  $kring = \UberH3::kRingDistances(603537747495354367, 1);
  assert_int_eq3(count($kring), 7);
  assert_int_eq3($kring[0][0], 603537747495354367);
  assert_int_eq3($kring[0][1], 0);
  assert_int_eq3($kring[1][0], 603537753803587583);
  assert_int_eq3($kring[1][1], 1);
  assert_int_eq3($kring[2][0], 603537753266716671);
  assert_int_eq3($kring[2][1], 1);
  assert_int_eq3($kring[3][0], 603537746958483455);
  assert_int_eq3($kring[3][1], 1);
  assert_int_eq3($kring[4][0], 603537746690047999);
  assert_int_eq3($kring[4][1], 1);
  assert_int_eq3($kring[5][0], 603537747226918911);
  assert_int_eq3($kring[5][1], 1);
  assert_int_eq3($kring[6][0], 603537751387668479);
  assert_int_eq3($kring[6][1], 1);
}

function test_hexRange() {
  assert_true(\UberH3::hexRange(603537747495354367, -1) === false);

  assert_array_int_eq3(\UberH3::hexRange(603537747495354367, 0), [603537747495354367]);
  assert_array_int_eq3(\UberH3::hexRange(603537747495354367, 1), [
    603537747495354367,
    603537753803587583,
    603537753266716671,
    603537746958483455,
    603537746690047999,
    603537747226918911,
    603537751387668479
  ]);
}

function test_hexRangeDistances() {
  assert_true(\UberH3::hexRangeDistances(603537747495354367, -1) === false);

  $kring = \UberH3::hexRangeDistances(603537747495354367, 0);
  assert_int_eq3(count($kring), 1);
  assert_int_eq3($kring[0][0], 603537747495354367);
  assert_int_eq3($kring[0][1], 0);

  $kring = \UberH3::hexRangeDistances(603537747495354367, 1);
  assert_int_eq3(count($kring), 7);
  assert_int_eq3($kring[0][0], 603537747495354367);
  assert_int_eq3($kring[0][1], 0);
  assert_int_eq3($kring[1][0], 603537753803587583);
  assert_int_eq3($kring[1][1], 1);
  assert_int_eq3($kring[2][0], 603537753266716671);
  assert_int_eq3($kring[2][1], 1);
  assert_int_eq3($kring[3][0], 603537746958483455);
  assert_int_eq3($kring[3][1], 1);
  assert_int_eq3($kring[4][0], 603537746690047999);
  assert_int_eq3($kring[4][1], 1);
  assert_int_eq3($kring[5][0], 603537747226918911);
  assert_int_eq3($kring[5][1], 1);
  assert_int_eq3($kring[6][0], 603537751387668479);
  assert_int_eq3($kring[6][1], 1);
}

function test_hexRanges() {
  assert_true(\UberH3::hexRanges([603537747495354367, 603537753266716671], -1) === false);

  assert_array_int_eq3(\UberH3::hexRanges([], 0), []);

  assert_array_int_eq3(\UberH3::hexRanges([603537747495354367, 603537753266716671], 0),
                       [603537747495354367, 603537753266716671]);
  assert_array_int_eq3(\UberH3::hexRanges([603537747495354367, 603537753266716671], 1), [
    603537747495354367,
    603537753803587583,
    603537753266716671,
    603537746958483455,
    603537746690047999,
    603537747226918911,
    603537751387668479,
    603537753266716671,
    603537753132498943,
    603537753535152127,
    603537749374402559,
    603537746958483455,
    603537747495354367,
    603537753803587583
  ]);
}

function test_hexRing() {
  assert_true(\UberH3::hexRing(603537747495354367, -1) === false);

  assert_array_int_eq3(\UberH3::hexRing(603537747495354367, 0), [603537747495354367]);
  assert_array_int_eq3(\UberH3::hexRing(603537747495354367, 1), [
    603537751387668479,
    603537753803587583,
    603537753266716671,
    603537746958483455,
    603537746690047999,
    603537747226918911
  ]);
}

function test_h3Line() {
  assert_array_int_eq3(\UberH3::h3Line(603537747495354367, 603537747495354367),
                       [603537747495354367]);
  assert_array_int_eq3(\UberH3::h3Line(603537747495354367, 603537747495354368),
                       [603537747495354367, 603537746690047999]);
  assert_array_int_eq3(\UberH3::h3Line(12412312, 124123123),
                       [576495936675512319]);
}

function test_h3LineSize() {
  assert_int_eq3(\UberH3::h3LineSize(603537747495354367, 603537747495354367), 1);
  assert_int_eq3(\UberH3::h3LineSize(603537747495354367, 603537747495354368), 2);
  assert_int_eq3(\UberH3::h3LineSize(12412312, 124123123), 1);
}

function test_h3Distance() {
  assert_int_eq3(\UberH3::h3Distance(603537747495354367, 603537747495354367), 0);
  assert_int_eq3(\UberH3::h3Distance(603537747495354367, 603537747495354368), 1);
  assert_int_eq3(\UberH3::h3Distance(12412312, 124123123), 0);
}

test_kRing();
test_maxKringSize();
test_kRingDistances();
test_hexRange();
test_hexRangeDistances();
test_hexRanges();
test_hexRing();
test_h3Line();
test_h3LineSize();
test_h3Distance();
