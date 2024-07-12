@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class A {
  public $x = 1;
}

function test_empty_string_default_value() {
  class EmptyStringDefault {
    const EMPTY = "";

    public string $empty_str1 = "";
    public string $empty_str2 = EmptyStringDefault::EMPTY;
    public string $empty_str3;

    public function __construct() {
      $this->empty_str3 = "";
    }
  }

  var_dump(to_array_debug(new EmptyStringDefault));
}

function test_optional_string_default_value() {
  class OptionalStringDefault {
    const EMPTY = "";

    /** @var string|false */
    public $str_or_false1 = false;
    /** @var string|false */
    public $str_or_false2 = "";
    /** @var string|false */
    public $str_or_false3 = OptionalStringDefault::EMPTY;

    public ?string $str_or_null1 = null;
    public ?string $str_or_null2 = "";
    public ?string $str_or_null3 = OptionalStringDefault::EMPTY;
    public ?string $str_or_null4;
  }

  $o = new OptionalStringDefault;
  // since php 7.4 (with class fields), not uninialized fields are not dumped with (array)$obj
  // that's why (array)$o won't output $str_or_null4 if not set, which will diff with kphp
  #ifndef KPHP
  $o->str_or_null4 = null;
  #endif
  var_dump(to_array_debug($o));
}

function test_empty_array_default_value() {
  class EmptyArrayDefault {
    const EMPTY = [];

    /** @var array */
    public $empty_arr1 = [];
    /** @var array */
    public $empty_arr2 = EmptyArrayDefault::EMPTY;
    /** @var array */
    public $empty_arr3;

    /** @var string[] */
    public $empty_arr_str1 = [];
    /** @var int[] */
    public $empty_arr_int2 = EmptyArrayDefault::EMPTY;
    /** @var A[] */
    public $empty_arr_A3;

    public function __construct() {
      $this->empty_arr3 = [];
      $this->empty_arr_A3 = [];
    }
  }

  var_dump(to_array_debug(new EmptyArrayDefault));
}

function test_optional_array_default_value() {
  class OptionalArrayDefault {
    const EMPTY = [];

    /** @var string[]|false */
    public $arr_str_or_false1 = false;
    /** @var int[]|false */
    public $arr_int_or_false2 = [];
    /** @var A[]|false */
    public $arr_A_or_false3 = OptionalArrayDefault::EMPTY;

    /** @var string[]|null */
    public $arr_str_or_null1 = null;
    /** @var int[]|null */
    public $arr_int_or_null2 = [];
    /** @var A[]|null */
    public $arr_A_or_null3 = OptionalArrayDefault::EMPTY;
    /** @var A[][]|null */
    public $arr_arr_A_or_null4;
  }

  var_dump(to_array_debug(new OptionalArrayDefault));
}


test_empty_string_default_value();
test_optional_string_default_value();
test_empty_array_default_value();
test_optional_array_default_value();
