@ok k2_skip
<?php

class TestConstants {
  const integer_1 = 1;
  const integer_2 = 2;
  const string_1 = "hello";
  const array_1 = [1, 2, 3, 4];
  const array_2 = ["1", "2", "3", "4"];
  const array_3 = ["1"."2"."3"."4"];

  static public $a = [ self::integer_1 ];
  static public $b = [ 1 | 2 ];
  static public $c = [ self::integer_1 | self::integer_2 ];

  static public $mixed_keys = [ 1, 2, "aaa" => "bbb" ];
  static public $const_elements1 = [self::string_1 => self::array_1];
  static public $const_elements2 = [
    self::array_1[self::integer_1] => self::string_1,
    self::array_2[self::integer_2] => self::array_3
  ];
  static public $const_elements3 = [self::array_3[0] => "hello"."world"];

  const const_elements4 = self::array_1[0];
  const const_elements5 = self::array_2[0] + self::array_2[3];
  const const_elements6 = [
    self::array_1[self::integer_1] => self::string_1,
    self::array_2[self::integer_2] => self::array_3
  ];
  const const_elements7 = [self::array_3[0] => "hello"."world"];
}

function test_concat() {
    $x = "123"."312";
    $y = ["aaa"."bbb" => "sss"."aaa"];

    var_dump($x);
    var_dump($y);
}

function test_class_constants() {
  var_dump(TestConstants::$a);
  var_dump(TestConstants::$b);
  var_dump(TestConstants::$c);

  var_dump(TestConstants::$mixed_keys);
  var_dump(TestConstants::$const_elements1);
  var_dump(TestConstants::$const_elements2);
  var_dump(TestConstants::$const_elements3);

  var_dump(TestConstants::const_elements4);
  var_dump(TestConstants::const_elements5);
  var_dump(TestConstants::const_elements6);
  var_dump(TestConstants::const_elements7);
}

function test_array_with_class_constants() {
  $x = [
    "key1" => TestConstants::array_1[TestConstants::integer_1],
    TestConstants::array_3[0] => "value"
  ];
  var_dump($x);
}

function test_const_regex() {
    var_dump (preg_replace ('~a'.'|~', 'b', 'c'));
    var_dump (preg_replace ('~a|~', 'b', 'c'));
}

function test_deep_array() {
    $x = [
        "one" => [1, 2, 3],
        "two" => [
            "hello" => "world",
            "3" => [1, 2, 3, 4]
        ],
        "three" => "some element",
        "four" => "other element"
    ];
    var_dump($x);
}

function test_array_in_default_param($x = [
  "param1" => [
    "hello" => ["foo"."bar", 123, "1"."2" => "3"."2"],
    TestConstants::array_3[0] => TestConstants::array_1[TestConstants::integer_1]
  ]
]) {
    var_dump($x);
}

test_concat();
test_class_constants();
test_array_with_class_constants();
test_const_regex();
test_deep_array();
test_array_in_default_param();
