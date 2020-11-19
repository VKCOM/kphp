@ok
<?php
// based on https://github.com/php/php-src/blob/master/tests/lang/operators/operator_spaceship_basic.phpt
$valid_true = array(1, "1", "true", 1.0, array(1));
$valid_false = array(0, "", 0.0, array(), NULL);

$int1 = 679;
$int2 = -67835;
$valid_int1 = array("678", "678abc", " 678", "678  ", 678.0, 6.789E2, "+678", +678);
$valid_int2 = array("-67836", "-67836abc", " -67836", "-67836  ", -67835.0001, -6.78351E4);
$invalid_int1 = array(679, "679");
$invalid_int2 = array(-67835, "-67835");

$float1 = 57385.45835;
$float2 = -67345.76567;
$valid_float1 = array("57385.45834",  "57385.45834aaa", "  57385.45834", 5.738545834e4);
$valid_float2 = array("-67345.76568", "-67345.76568aaa", "  -67345.76568", -6.734576568E4);
$invalid_float1 = array(57385.45835, 5.738545835e4);
$invalid_float2 = array(-67345.76567, -6.734576567E4);


$toCompare = array(
// boolean test will result in both sides being converted to boolean so !0 = true and true is not > true for example
// also note that a string of "0" is converted to false but a string of "0.0" is converted to true
// false cannot be tested as 0 can never be > 0 or 1
    true, $valid_false, $valid_true,
    $int1, $valid_int1, $invalid_int1,
    $int2, $valid_int2, $invalid_int2,
    $float1, $valid_float1, $invalid_float1,
    $float2, $valid_float2, $invalid_float2
);

$failed = false;
for ($i = 0; $i < count($toCompare); $i +=3) {
  $typeToTest = $toCompare[$i];
  $valid_compares = $toCompare[$i + 1];
  $invalid_compares = $toCompare[$i + 2];

  foreach($valid_compares as $compareVal) {
    echo $typeToTest <=> $compareVal, PHP_EOL;
  }

  foreach($invalid_compares as $compareVal) {
    echo $typeToTest <=> $compareVal, PHP_EOL;
  }

  echo $typeToTest <=> $typeToTest, PHP_EOL;
}

// additional cases for kphp
$additionalCases = [
    ["a", "a"],  // 0
    ["a", "b"],  // -1
    ["b", "a"],  // 1
    ["a", "aa"], // -1
    ["zz", "aa"], // 1
    ["40", 30], // 1
];

foreach ($additionalCases as $case) {
  echo $case[0] <=> $case[1], PHP_EOL;
}

$a = [] <=> [];
$b = [1,2,3] <=> [1,2,3];
$c = [1,2,3] <=> [];
$d = [1,2,3] <=> [1,2,1];
$e = [1,2,3] <=> [1,2,4];
echo $a, $b, $c, $d, $e;
