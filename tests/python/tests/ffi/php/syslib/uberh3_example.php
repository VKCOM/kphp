<?php

#ifndef KPHP
function tuple(...$args) {
  return $args;
}

define('kphp', 0);
if (false)
#endif
  define('kphp', 1);

function die_if_failure(bool $is_ok, string $msg) {
  if (!$is_ok) {
    $failure_msg = "Failure\n$msg\n";
#ifndef KPHP
    $failure_msg .= (new \Exception)->getTraceAsString();
#endif
    warning($failure_msg);
    die(1);
  }
}

function assert_false(bool $value) {
  echo "assert_false($value)\n";
  die_if_failure(!$value, "Expected false");
}

function assert_true(bool $value) {
  echo "assert_true($value)\n";
  die_if_failure($value, "Expected true");
}

function assert_int_eq3(int $lhs, int $rhs) {
  echo "assert_int_eq3($lhs, $rhs)\n";
  die_if_failure($lhs === $rhs, "Expected equality of these values: $lhs and $rhs");
}

function assert_near(float $lhs, float $rhs, float $abs_error = 1e-6) {
  echo "assert_near($lhs, $rhs, $abs_error)\n";
  $diff = abs($lhs - $rhs);
  die_if_failure(abs($lhs - $rhs) <= $abs_error,
                 "The difference between $lhs and $rhs is $diff, which exceeds $abs_error");
}

/**
 * @param $lhs tuple(float, float)
 * @param $rhs tuple(float, float)
 */
function assert_lat_lon($lhs, $rhs) {
  assert_near($lhs[0], $rhs[0]);
  assert_near($lhs[1], $rhs[1]);
}

class H3 {
  /** @var ffi_scope<h3> */
  private $lib;

  public function __construct() {
    $this->lib = FFI::cdef('
      #define FFI_SCOPE "h3"
      typedef struct {
        double lat;
        double lon;
      } GeoCoord;

      typedef uint64_t H3Index;

      double degsToRads(double degrees);
      double radsToDegs(double radians);

      H3Index geoToH3(const GeoCoord *g, int res);
      void h3ToGeo(H3Index h3, GeoCoord *g);

      int h3GetResolution(H3Index h);
      int h3GetBaseCell(H3Index h);
      H3Index stringToH3(const char *str);
      int h3IsValid(H3Index h);
      int h3IsResClassIII(H3Index h);
      int h3IsPentagon(H3Index h);
      int maxFaceCount(H3Index h3);
    ', 'libh3.so');
  }

  public function geoToH3(float $latitude, float $longitude, int $resolution): int {
    $coord = $this->lib->new('GeoCoord');
    $coord->lat = $this->lib->degsToRads($latitude);
    $coord->lon = $this->lib->degsToRads($longitude);
    return $this->lib->geoToH3(FFI::addr($coord), $resolution);
  }

  /** @return tuple(float, float) */
  public function h3ToGeo(int $h3_index) {
    $coord = $this->lib->new('GeoCoord');
    $this->lib->h3ToGeo($h3_index, FFI::addr($coord));
    $latitude = $this->lib->radsToDegs($coord->lat);
    $longitude = $this->lib->radsToDegs($coord->lon);
    return tuple($latitude, $longitude);
  }

  public function h3GetResolution(int $h3_index): int {
    return $this->lib->h3GetResolution($h3_index);
  }

  public function h3GetBaseCell(int $h3_index): int {
    return $this->lib->h3GetBaseCell($h3_index);
  }

  public function stringToH3(string $h3_index_str): int {
    return $this->lib->stringToH3($h3_index_str);
  }

  public function h3IsValid(int $h3_index): bool {
    return (bool)$this->lib->h3IsValid($h3_index);
  }

  public function h3IsResClassIII(int $h3_index): bool {
    return (bool)$this->lib->h3IsResClassIII($h3_index);
  }

  public function h3IsPentagon(int $h3_index): bool {
    return (bool)$this->lib->h3IsPentagon($h3_index);
  }

  public function maxFaceCount(int $h3_index): int {
    return $this->lib->maxFaceCount($h3_index);
  }
}

function test_geoToH3(H3 $h3) {
  assert_int_eq3($h3->geoToH3(0, 0, 16), 0);
  assert_int_eq3($h3->geoToH3(0, 0, -1), 0);

  assert_int_eq3($h3->geoToH3(0, 0, 0), 578536630256664575);
  assert_int_eq3($h3->geoToH3(0, 0, 1), 583031433791012863);
  assert_int_eq3($h3->geoToH3(0, 0, 2), 587531734883500031);
  assert_int_eq3($h3->geoToH3(-0.45969564234129, 0.53416786298236, 2), 587531734883500031);
  assert_int_eq3($h3->geoToH3(0, 0, 15), 646078419604526808);

  assert_int_eq3($h3->geoToH3(30, 50, 11), 626785114471653375);
  assert_int_eq3($h3->geoToH3(30.000045196706, 50.00011997391, 11), 626785114471653375);
  assert_int_eq3($h3->geoToH3(70, 120, 7), 608083127927046143);
  assert_int_eq3($h3->geoToH3(90, 90, 6), 603537747495354367);
  assert_int_eq3($h3->geoToH3(70, 120, 3), 590068789245116415);
}

function test_h3ToGeo(H3 $h3) {
  assert_lat_lon($h3->h3ToGeo(0), tuple(79.242398509759, 38.02340700797));
  assert_lat_lon($h3->h3ToGeo(1), tuple(79.242398509759, 38.02340700797));
  assert_lat_lon($h3->h3ToGeo(2), tuple(79.242398509759, 38.02340700797));

  assert_lat_lon($h3->h3ToGeo(578536630256664575), tuple(2.3008821116267, -5.2453902967773));
  assert_lat_lon($h3->h3ToGeo(587531734883500031), tuple(-0.45969564234129, 0.53416786298236));
  assert_lat_lon($h3->h3ToGeo(626785114471653375), tuple(30.000045196706, 50.00011997391));
}

function test_h3GetResolution(H3 $h3) {
  assert_int_eq3($h3->h3GetResolution(0), 0);
  assert_int_eq3($h3->h3GetResolution(1), 0);

  assert_int_eq3($h3->h3GetResolution(608083127927046143), 7);
  assert_int_eq3($h3->h3GetResolution(587531734883500031), 2);

  assert_int_eq3($h3->h3GetResolution($h3->geoToH3(0, 11, 0)), 0);
  assert_int_eq3($h3->h3GetResolution($h3->geoToH3(70, 12, 1)), 1);
  assert_int_eq3($h3->h3GetResolution($h3->geoToH3(15, 12, 2)), 2);
  assert_int_eq3($h3->h3GetResolution($h3->geoToH3(70, 120, 3)), 3);
  assert_int_eq3($h3->h3GetResolution($h3->geoToH3(32, 41, 8)), 8);
  assert_int_eq3($h3->h3GetResolution($h3->geoToH3(4, 4, 15)), 15);
}

function test_h3GetBaseCell(H3 $h3) {
  assert_int_eq3($h3->h3GetBaseCell(0), 0);
  assert_int_eq3($h3->h3GetBaseCell(1), 0);

  assert_int_eq3($h3->h3GetBaseCell(608083127927046143), 2);
  assert_int_eq3($h3->h3GetBaseCell(587531734883500031), 58);

  assert_int_eq3($h3->h3GetBaseCell($h3->geoToH3(0, 11, 0)), 65);
  assert_int_eq3($h3->h3GetBaseCell($h3->geoToH3(70, 12, 1)), 4);
  assert_int_eq3($h3->h3GetBaseCell($h3->geoToH3(15, 12, 2)), 44);
  assert_int_eq3($h3->h3GetBaseCell($h3->geoToH3(70, 120, 3)), 2);
  assert_int_eq3($h3->h3GetBaseCell($h3->geoToH3(32, 41, 8)), 22);
  assert_int_eq3($h3->h3GetBaseCell($h3->geoToH3(4, 4, 15)), 65);
}

function test_stringToH3(H3 $h3) {
  assert_int_eq3($h3->stringToH3("0"), 0);
  assert_int_eq3($h3->stringToH3("1"), 1);

  assert_int_eq3($h3->stringToH3("870586211ffffff"), 608083127927046143);
  assert_int_eq3($h3->stringToH3("82754ffffffffff"), 587531734883500031);
}

function test_h3IsValid(H3 $h3) {
  assert_false($h3->h3IsValid(0));
  assert_false($h3->h3IsValid(1));

  assert_true($h3->h3IsValid(578536630256664575));
  assert_true($h3->h3IsValid(587531734883500031));

  assert_true($h3->h3IsValid($h3->geoToH3(70, 120, 3)));
  assert_true($h3->h3IsValid($h3->geoToH3(-10, 14, 7)));
}

function test_h3IsResClassIII(H3 $h3) {
  assert_false($h3->h3IsResClassIII(0));
  assert_false($h3->h3IsResClassIII(1));

  assert_true($h3->h3IsResClassIII(608083127927046143));
  assert_false($h3->h3IsResClassIII(603537747495354367));

  assert_true($h3->h3IsResClassIII($h3->geoToH3(0, 0, 3)));
  assert_false($h3->h3IsResClassIII($h3->geoToH3(0, 0, 4)));
}

function test_h3IsPentagon(H3 $h3) {
  assert_false($h3->h3IsPentagon(0));
  assert_false($h3->h3IsPentagon(1));

  assert_false($h3->h3IsPentagon(608083127927046143));
  assert_false($h3->h3IsPentagon(603537747495354367));

  assert_false($h3->h3IsPentagon($h3->geoToH3(0, 0, 3)));
  assert_false($h3->h3IsPentagon($h3->geoToH3(0, 0, 4)));
}

function test_maxFaceCount(H3 $h3) {
  assert_int_eq3($h3->maxFaceCount(608083127927046143), 2);
  assert_int_eq3($h3->maxFaceCount($h3->geoToH3(0, 0, 3)), 2);
}

$h3 = new H3();
test_geoToH3($h3);
test_h3ToGeo($h3);
test_h3GetResolution($h3);
test_h3GetBaseCell($h3);
test_stringToH3($h3);
test_h3IsValid($h3);
test_h3IsResClassIII($h3);
test_h3IsPentagon($h3);
test_maxFaceCount($h3);
