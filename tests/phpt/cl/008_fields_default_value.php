@ok
<?php

require_once "polyfills.php";

class A {
  public $x = 1;
}

function test_empty_string_default_value() {
  class EmptyStringDefault {
    const EMPTY = "";

    /** @var string */
    public $empty_str1 = "";
    /** @var string */
    public $empty_str2 = EmptyStringDefault::EMPTY;
    /** @var string */
    public $empty_str3;

    public function __construct() {
      $this->empty_str3 = "";
    }
  }

  var_dump(instance_to_array(new EmptyStringDefault));
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

    /** @var string|null */
    public $str_or_null1 = null;
    /** @var string|null */
    public $str_or_null2 = "";
    /** @var string|null */
    public $str_or_null3 = OptionalStringDefault::EMPTY;
    /** @var string|null */
    public $str_or_null4;
  }

  var_dump(instance_to_array(new OptionalStringDefault));
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

  var_dump(instance_to_array(new EmptyArrayDefault));
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

  var_dump(instance_to_array(new OptionalArrayDefault));
}


test_empty_string_default_value();
test_optional_string_default_value();
test_empty_array_default_value();
test_optional_array_default_value();
