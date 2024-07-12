@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class S {
    function __toString() { return "S"; }
}

/**
 * @kphp-generic TArray
 * @param TArray $arr
 */
function f($arr) {
    /** @var tuple(int, TArray) */
    $tup = tuple(0, $arr);
    foreach ($tup[1] as $i)
        echo $i, "\n";
}

/**
 * @kphp-generic T
 * @param T $first
 * @param T $second
 */
function g($first, $second) {
    f/*<T[]>*/([$first, $second]);
}

g/*<int>*/(1, 2);
g/*<string>*/('a', 'b');
g/*<S>*/(new S, new S);


class User {
    public int $age;

    function __construct(int $age) { $this->age = $age; }

    function isOlder(User $r): bool { return $this->age > $r->age; }
}


/**
 * @kphp-generic T
 * @param callable(T,T): bool $gt
 * @param T ...$arr
 * @return T
 */
function maxBy($gt, ...$arr) {
    $max = array_first_value($arr);
    for ($i = 1; $i < count($arr); ++$i) {
        if ($gt($arr[$i], $max))
            $max = $arr[$i];
    }
    return $max;
}

echo maxBy/*<int>*/(fn ($a, $b) => $a > $b, 1, 2, 9, 3), "\n";
echo maxBy/*<string>*/(fn ($a, $b) => ord($a) > ord($b), 'a', 'z', 'd'), "\n";
echo maxBy/*<User>*/(fn ($a, $b) => $a->isOlder($b), new User(8), new User(10), new User(7))->age, "\n";


class Fields {
    static public int $S_I = 0;

    public int $i = 0;
    /** @var string[] */
    public array $s = ['s'];
    public ?User $u = null;

    /** @return string[] */
    function getS(): array {
        return $this->s;
    }

    function getI(): int {
        return $this->i;
    }
}

function justGetBool(): bool {
    return true;
}
function getBoolNoHint() {
    return true;
}


/**
 * @kphp-generic T
 * @param T $some
 */
function myVarDump($some) {
    var_dump($some);
}

/**
 * @kphp-generic T
 * @param T $u
 */
function myCallUser($u) {
    myVarDump($u->isOlder($u));
}

$f = new Fields;
$f->u = new User(10);

myVarDump([1,2,3]);
myVarDump(Fields::$S_I);
myVarDump($f->i);
myVarDump($f->u->age);
myVarDump($f->s);
myVarDump($f->getI());
myVarDump($f->getS());
myVarDump($f->getS()[0]);
myVarDump(justGetBool());
myVarDump(getBoolNoHint());

myCallUser($f->u);

/**
 * @kphp-generic T
 * @param T[] $arr
 */
function testAutoReifyArr($arr) {
    echo "arr count ", count($arr), "\n";
}

testAutoReifyArr([1,2,3]);
testAutoReifyArr(['s1', 's2']);
testAutoReifyArr/*<?string>*/(['s1', 's2', null]);

/** @var int[] */
$global_ii = [];
testAutoReifyArr($global_ii);

function fWithPrimitive(int $i) {
    /** @var int $i */
    myVarDump($i);
}
fWithPrimitive(100);
