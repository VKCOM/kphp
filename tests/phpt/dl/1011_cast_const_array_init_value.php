@ok k2_skip
KPHP_ERROR_ON_WARNINGS=1
<?php

class MyClass1 {
    public $x = 1;
}

class MyClass2 {
    public $y = 1;
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function test_static_arrays() {
    static $unknown_arr = [];
    var_dump($unknown_arr);

    static $int_arr1 = [];
    $int_arr1[] = 2;
    var_dump($int_arr1);

    static $int_arr2 = [1, 2, 3];
    $int_arr2[] = 4;
    var_dump($int_arr2);

    static $var_val = [1, 2, 3];
    $var_val = "make_me_var";
    var_dump($var_val);

    static $var_arr = [1, 2, 3];
    $var_arr[] = "make_me_var";
    var_dump($var_arr);

    static $unknown_arr_arr = [[]];
    var_dump($unknown_arr_arr);

    static $int_arr_arr = [[]];
    $int_arr_arr[] = [1];
    var_dump($int_arr_arr);

    static $or_false_arr1 = false;
    if (1) { $or_false_arr1 = [1, 2, 3]; }
    var_dump ($or_false_arr1);

    static $or_false_arr2 = [1, 2, 3];
    if (0) { $or_false_arr2 = false; }
    $or_false_arr2[] = "make me var";
    var_dump($or_false_arr2);

    static $arr_arr_or_false1 = [false];
    if (1) { $arr_arr_or_false1[0] = [1, 2, 3]; }
    var_dump($arr_arr_or_false1);

    static $arr_arr_or_false2 = [[1, 2, 3]];
    if (0) { $arr_arr_or_false2[0] = false; }
    array_push($arr_arr_or_false2[0], "make me var");
    var_dump($arr_arr_or_false2);

    static $arr_class1 = [];
    $arr_class1[] = new MyClass1();
    var_dump(get_class($arr_class1[0]));

    static $arr_class2 = [];
    $arr_class2[] = new MyClass2();
    var_dump(get_class($arr_class2[0]));
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function test_local_arrays() {
    $unknown_arr = [];
    var_dump($unknown_arr);

    $int_arr1 = [];
    $int_arr1[] = 2;
    var_dump($int_arr1);

    $int_arr2 = [1, 2, 3];
    $int_arr2[] = 4;
    var_dump($int_arr2);

    $var_val = [1, 2, 3];
    $var_val = "make_me_var";
    var_dump($var_val);

    $var_arr = [1, 2, 3];
    $var_arr[] = "make_me_var";
    var_dump($var_arr);

    $unknown_arr_arr = [[]];
    var_dump($unknown_arr_arr);

    $int_arr_arr = [[]];
    $int_arr_arr[] = [1];
    var_dump($int_arr_arr);

    $or_false_arr1 = false;
    if (1) { $or_false_arr1 = [1, 2, 3]; }
    var_dump ($or_false_arr1);

    $or_false_arr2 = [1, 2, 3];
    if (0) { $or_false_arr2 = false; }
    $or_false_arr2[] = "make me var";
    var_dump($or_false_arr2);

    $arr_arr_or_false1 = [false];
    if (1) { $arr_arr_or_false1[0] = [1, 2, 3]; }
    var_dump($arr_arr_or_false1);

    $arr_arr_or_false2 = [[1, 2, 3]];
    if (0) { $arr_arr_or_false2[0] = false; }
    array_push($arr_arr_or_false2[0], "make me var");
    var_dump($arr_arr_or_false2);

    $arr_class1 = [];
    $arr_class1[] = new MyClass1();
    var_dump(get_class($arr_class1[0]));

    $arr_class2 = [];
    $arr_class2[] = new MyClass2();
    var_dump(get_class($arr_class2[0]));
}

/**
 *
 * @kphp-warn-performance implicit-array-cast
 *
 * @param any[] $unknown_arr
 * @param int[] $int_arr1
 * @param int[] $int_arr2
 * @param int[] $var_val
 * @param mixed[] $var_arr
 * @param any[][] $unknown_arr_arr
 * @param int[][] $int_arr_arr
 * @param int[]|false $or_false_arr1
 * @param mixed[]|false $or_false_arr2
 * @param (int[]|false)[] $arr_arr_or_false1
 * @param (mixed[]|false)[] $arr_arr_or_false2
 * @param MyClass1[] $arr_class1
 * @param MyClass2[] $arr_class2
 */
function test_default_params_arrays(
    $unknown_arr = [],
    $int_arr1 = [],
    $int_arr2 = [1, 2, 3],
    $var_val = [1, 2, 3],
    $var_arr = [1, 2, 3],
    $unknown_arr_arr = [[]],
    $int_arr_arr = [[]],
    $or_false_arr1 = false,
    $or_false_arr2 = [1, 2, 3],
    $arr_arr_or_false1 = [false],
    $arr_arr_or_false2 = [[1, 2, 3]],
    $arr_class1 = [],
    $arr_class2 = []
) {
    var_dump($unknown_arr);

    $int_arr1[] = 2;
    var_dump($int_arr1);

    $int_arr2[] = 4;
    var_dump($int_arr2);

    $var_val = "make_me_var";
    var_dump($var_val);

    $var_arr[] = "make_me_var";
    var_dump($var_arr);

    var_dump($unknown_arr_arr);

    $int_arr_arr[] = [1];
    var_dump($int_arr_arr);

    if (1) { $or_false_arr1 = [1, 2, 3]; }
    var_dump ($or_false_arr1);

    if (0) { $or_false_arr2 = false; }
    $or_false_arr2[] = "make me var";
    var_dump($or_false_arr2);

    if (1) { $arr_arr_or_false1[0] = [1, 2, 3]; }
    var_dump($arr_arr_or_false1);

    if (0) { $arr_arr_or_false2[0] = false; }
    array_push($arr_arr_or_false2[0], "make me var");
    var_dump($arr_arr_or_false2);

    $arr_class1[] = new MyClass1();
    var_dump(get_class($arr_class1[0]));

    $arr_class2[] = new MyClass2();
    var_dump(get_class($arr_class2[0]));
}

class VarPack {
    static public $unknown_arr = [];
    static public $int_arr1 = [];
    static public $int_arr2 = [1, 2, 3];
    /** @var mixed */
    static public $var_val = [1, 2, 3];
    /** @var mixed[] */
    static public $var_arr = [1, 2, 3];
    static public $unknown_arr_arr = [[]];
    static public $int_arr_arr = [[]];
    /** @var int[]|false */
    static public $or_false_arr1 = false;
    /** @var mixed[]|false */
    static public $or_false_arr2 = [1, 2, 3];
    /** @var (int[]|false)[] */
    static public $arr_arr_or_false1 = [false];
    /** @var (mixed[]|false)[] */
    static public $arr_arr_or_false2 = [[1, 2, 3]];

    static public $arr_class1 = [];
    static public $arr_class2 = [];
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function test_static_class_member_arrays() {
    var_dump(VarPack::$unknown_arr);

    VarPack::$int_arr1[] = 2;
    var_dump(VarPack::$int_arr1);

    VarPack::$int_arr2[] = 4;
    var_dump(VarPack::$int_arr2);

    VarPack::$var_val = "make_me_var";
    var_dump(VarPack::$var_val);

    VarPack::$var_arr[] = "make_me_var";
    var_dump(VarPack::$var_arr);

    var_dump(VarPack::$unknown_arr_arr);

    VarPack::$int_arr_arr[] = [1];
    var_dump(VarPack::$int_arr_arr);

    if (1) { VarPack::$or_false_arr1 = [1, 2, 3]; }
    var_dump (VarPack::$or_false_arr1);

    if (0) { VarPack::$or_false_arr2 = false; }
    VarPack::$or_false_arr2[] = "make me var";
    var_dump(VarPack::$or_false_arr2);

    if (1) { VarPack::$arr_arr_or_false1[0] = [1, 2, 3]; }
    var_dump(VarPack::$arr_arr_or_false1);

    if (0) { VarPack::$arr_arr_or_false2[0] = false; }
    array_push(VarPack::$arr_arr_or_false2[0], "make me var");
    var_dump(VarPack::$arr_arr_or_false2);

    VarPack::$arr_class1[] = new MyClass1();
    var_dump(get_class(VarPack::$arr_class1[0]));

    VarPack::$arr_class2[] = new MyClass2();
    var_dump(get_class(VarPack::$arr_class2[0]));
}

/**
 *
 * @kphp-warn-performance implicit-array-cast
 *
 * @param any[] $unknown_arr
 * @param int[] $int_arr1
 * @param int[] $int_arr2
 * @param int[] $var_val
 * @param mixed[] $var_arr
 * @param any[][] $unknown_arr_arr
 * @param int[][] $int_arr_arr
 * @param false $or_false_arr1
 * @param int[] $or_false_arr2
 * @param false[] $arr_arr_or_false1
 * @param int[][] $arr_arr_or_false2
 * @param MyClass1[] $arr_class1
 * @param MyClass2[] $arr_class2
 */
function dummy_func(
    $unknown_arr,
    $int_arr1,
    $int_arr2,
    $var_val,
    $var_arr,
    $unknown_arr_arr,
    $int_arr_arr,
    $or_false_arr1,
    $or_false_arr2,
    $arr_arr_or_false1,
    $arr_arr_or_false2,
    $arr_class1,
    $arr_class2
) {
    var_dump($unknown_arr);

    $int_arr1[] = 2;
    var_dump($int_arr1);

    $int_arr2[] = 4;
    var_dump($int_arr2);

    $var_val = "make_me_var";
    var_dump($var_val);

    $var_arr[] = "make_me_var";
    var_dump($var_arr);

    var_dump($unknown_arr_arr);

    $int_arr_arr[] = [1];
    var_dump($int_arr_arr);

    if (1) { VarPack::$or_false_arr1 = [1, 2, 3]; }
    var_dump (VarPack::$or_false_arr1);

    if (0) { VarPack::$or_false_arr2 = false; }
    VarPack::$or_false_arr2[] = "make me var";
    var_dump(VarPack::$or_false_arr2);

    if (1) { VarPack::$arr_arr_or_false1[0] = [1, 2, 3]; }
    var_dump(VarPack::$arr_arr_or_false1);

    if (0) { VarPack::$arr_arr_or_false2[0] = false; }
    array_push(VarPack::$arr_arr_or_false2[0], "make me var");
    var_dump(VarPack::$arr_arr_or_false2);

    $arr_class1[] = new MyClass1();
    var_dump(get_class($arr_class1[0]));

    $arr_class2[] = new MyClass2();
    var_dump(get_class($arr_class2[0]));
}

/**
 * @kphp-warn-performance implicit-array-cast
 */
function test_func_arg_arrays() {
    dummy_func(
   /* $unknown_arr, */ [],
   /* $int_arr1, */ [],
   /* $int_arr2, */ [1, 2, 3],
   /* $var_val, */ [1, 2, 3],
   /* $var_arr, */ [1, 2, 3],
   /* $unknown_arr_arr, */ [[]],
   /* $int_arr_arr, */ [[]],
   /* $or_false_arr1, */ false,
   /* $or_false_arr2, */ [1, 2, 3],
   /* $arr_arr_or_false1, */ [false],
   /* $arr_arr_or_false2, */ [[1, 2, 3]],
   /* $arr_class1, */ [],
   /* $arr_class2 */ []);
}

test_static_arrays();
test_local_arrays();
test_default_params_arrays();
test_static_class_member_arrays();
test_func_arg_arrays();
