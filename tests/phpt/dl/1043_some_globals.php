@ok
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

$assigned_mixed = min(1, '2');

$int_s = 0;

function ccc1() {
    static $int_s = 0;
    echo $int_s, "\n";
}
function ccc2() {
    static $int_s = 0;
    echo $int_s, "\n";
}
ccc1();
ccc2();

class SurveyTarget {
    public int $target = 0;
}


/** @return SurveyTarget[][] */
function getEmptyArrayOfArraysOfClass(): array {
    return [
      'm' => [],
      'f' => [],
    ];
}

$targets = getEmptyArrayOfArraysOfClass();
echo 'count empty arr ', count($targets), "\n";

define('NOW', time());
if (0) {
    echo NOW, "\n";
}

// const_var
$s_concat = 'asdf' . '3';
echo $s_concat, "\n";
// const_var
$a_tuples = [tuple(1, true)];
echo count($a_tuples);
// const_var float
$float_sin_30 = sin(30);
echo $float_sin_30, "\n";
// const_var optional<float>
$float_optional_min = min(1.0, null);
echo $float_optional_min, "\n";
// const_var bool
$g_assigned_const_bool = min(true, false);
var_dump($g_assigned_const_bool);

function acceptsVariadict(...$args) {
    var_dump($args);
}
acceptsVariadict(''. 'str converted to array(1) into variadic');

/** @param mixed[] $arr */
function acceptArrayMixed($arr) {
    static $inAcceptArrayMixed = [0];

    echo count($arr);
    preg_match('/asdf+/', $arr[0]);
}

acceptArrayMixed([1,2,3]);
acceptArrayMixed([4, 5, 6]);

/** @var tuple(int, tuple(string|false, int|null)) */
$g_tuple = tuple(1, tuple('str', 10));
echo $g_tuple[1][0], "\n";
$g_tuple = tuple(1, tuple(false, null));
var_dump($g_tuple[1][1]); echo "\n";

/** @var tuple(bool) */
$g_tuple_2 = tuple(true);
var_dump($g_tuple_2[0]);
/** @var tuple(bool,bool) */
$g_tuple_3 = tuple(true,true);
var_dump($g_tuple_3[1]);
/** @var tuple(bool,int,bool) */
$g_tuple_4 = tuple(true,0,true);
var_dump($g_tuple_4[2]);
/** @var tuple(bool,bool,bool,bool,bool,bool,bool,bool,bool) */
$g_tuple_5 = tuple(true,true,true,true,true,true,true,false,true);
var_dump($g_tuple_5[7]);
var_dump($g_tuple_5[8]);

/** @var shape(x:?int, y:SurveyTarget) */
$g_shape = shape(['y' => new SurveyTarget]);
echo get_class($g_shape['y']), "\n";

function getInt(): int { return 5; }

/** @var future<int> */
$g_future = fork(getInt());
$g_future_result = wait($g_future);

/** @var ?future_queue<int> */
$g_future_queue = wait_queue_create([$g_future]);

function defArgConstants($s1 = 'str', $ar = [1,2,3]) {
    $a = [[[[1]]]][0];
    echo $s1, count($ar), "\n";
}
defArgConstants();
defArgConstants('str2', [1,2]);

class WithGlobals {
    static public $unused_and_untyped = null;
    static public $used_and_untyped = null;
    static public int $c1_int = 0;
    static public string $c1_string = 'asdf';
    /** @var ?SurveyTarget */
    static public $inst1 = null;
    /** @var ?tuple(int, bool, ?SurveyTarget[]) */
    static public $tup1 = null;
    /** @var ?tuple(bool) */
    static public $tup2 = null;
    /** @var ?tuple(bool,bool,bool,?int,bool,bool,bool,bool) */
    static public $tup3 = null;
    /** @var ?tuple(bool,bool,bool,bool,bool,bool,bool,bool,bool) */
    static public $tup4 = null;
    /** @var ?shape(single: tuple(bool, mixed, bool)) */
    static public $sh1 = null;
    /** @var Exception */
    static public $ex1 = null;
    /** @var ?LogicException */
    static public $ex2 = null;

    static function use() {
        self::$c1_string .= 'a' . 'b';
        self::$c1_int += 10;

        self::$tup1 = tuple(1, true, null);
        self::$tup1 = tuple(1, true, [new SurveyTarget]);
        self::$sh1 = shape([
            'single' => tuple(true, [1, 'str'], false),
        ]);
        self::$tup3 = tuple(true,true,true,null,true,true,false,true);
        self::$tup4 = tuple(true,true,true,true,true,true,true,false,true);
//         self::$ex1 = new Exception;
//         self::$ex2 = new LogicException;

        echo self::$used_and_untyped, "\n";
        echo self::$c1_int, "\n";
        echo self::$c1_string, "\n";
        echo self::$sh1['single'][1][1], "\n";

        var_dump(self::$tup2 === null);
        self::$tup2 = tuple(true);
        var_dump(self::$tup2[0]);

        var_dump(self::$tup3[6]);
        var_dump(self::$tup3[7]);

        var_dump(self::$tup4[7]);
        var_dump(self::$tup4[8]);
    }
}

WithGlobals::use();

global $g_unknown;

function accessUnknown() {
    global $g_unknown;
}

accessUnknown();

class WithUnusedMethod {
    static public int $used_only_in_unreachable = 123;
    function unreachableViaCfgMethod() {
        echo self::$used_only_in_unreachable, "\n";
    }
    function usedMethod() {
        throw new Exception;
        // this call will be deleted in cfg
        $this->unreachableViaCfgMethod();
    }
}

if (0) (new WithUnusedMethod)->usedMethod();

function useSuperglobals() {
    $_REQUEST = ['l' => 1];
    echo "count _REQUEST = ", count($_REQUEST), "\n";
    echo "php_sapi = ", PHP_SAPI, "\n";
}

useSuperglobals();

function toLowerPrint() {
  var_dump (mb_strtolower('ABCABC'));
}
toLowerPrint();

trait T {
    static public int $i = 0;

    public function incAndPrint() {
        self::$i++;
        echo get_class($this), " = ", self::$i, "\n";
    }
}

class TInst1 {
    use T;
}

(new TInst1)->incAndPrint();

class UnusedClass1 {
    static public $field1 = 0;
    static private $field2 = null;
}

class UnusedClass2 {
    use T;
}

$global_arr = [1,2,3];
foreach ($global_arr as &$global_item_ref) {
    $global_item_ref *= 2;
}
unset($global_item_ref);
echo "global_arr = ", implode(',', $global_arr), "\n";

class H { function str() { return 'str'; } }
function heredoc(): H { return new H; }

$html1 = 'div';
$html2 = 'span';
if (true) {
  $hd = heredoc();
  $html1 .= " title=\"{$hd->str()}\"";
  $html2 .= " header=\"{$hd->str()}\"";
}
echo $html1, ' ', $html2, "\n";

$htmls = ['i', 'b'];
if (true) {
  $hd = heredoc();
  foreach ($htmls as &$htmli) {
    $htmli .= " data-txt=\"{$hd->str()}\"";
  }
}
echo implode(' ', $htmls), "\n";

class SomeAnother22 {
    static public string $final_message = '';

    static function formAndPrint() {
        $owner_id = 1;
        SomeAnother22::$final_message .= "owner_id = $owner_id";
        echo SomeAnother22::$final_message, "\n";
    }
}

SomeAnother22::formAndPrint();

function declaresGlobalButNotUsesIt() {
    global $g_unknown, $g_assigned_const_bool;
    static $s_unused = 0;
    echo __FUNCTION__, "\n";
    return;
    echo $g_assigned_const_bool;
    echo $s_unused;
}

declaresGlobalButNotUsesIt();

class UsedInFunctionStaticOnly {
    public string $str = '';
}

function hasStaticInstance() {
    /** @var UsedInFunctionStaticOnly $inst */
    static $inst = null;
    if ($inst === null)
        $inst = new UsedInFunctionStaticOnly;
}
hasStaticInstance();

