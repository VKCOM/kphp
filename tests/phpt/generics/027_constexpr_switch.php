@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
    public int $value;

    function __construct(int $value) {
        $this->value = $value;
    }

    function printMe() {
        echo "A value=$this->value\n";
    }

    function inc() {
        $this->value++;
        return $this;
    }
}

class B {
    public string $marker;

    function __construct(string $marker) {
        $this->marker = $marker;
    }

    function printMe() {
        echo "B marker=$this->marker\n";
    }

    function append() {
        $this->marker .= '!';
        return $this;
    }
}

class Emptyn {}


/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return T
 */
function fact1(string $cn) {
    switch($cn) {
    case A::class:
        $obj = new A(10);
        $obj->inc();
        break;
    case B::class:
        $obj = new B('fact1');
        $obj->append();
        break;
    }

    $obj->printMe();
    return $obj;
}

fact1(A::class)->inc()->printMe();
fact1(B::class)->append()->printMe();


/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return T
 */
function fact2(string $cn) {
    switch($cn) {
    case A::class:
        $obj = new A(20);
        if ($obj->value == 20)
            break;
        $obj->inc();
        break;
    default:
        $obj = new B('fact2');
        $obj->append();
        break;
    }

    $obj->printMe();
    return $obj;
}

fact2(A::class)->inc()->printMe();
fact2(B::class)->append()->printMe();


/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return A
 */
function fact3(string $cn) {
    switch($cn) {
    case A::class:
    case B::class:
        echo "passed $cn, create A\n";
        $obj = new A(10);
        $obj->inc();
        break;
    }

    $obj->printMe();
    return $obj;
}

fact3(A::class)->inc()->printMe();
fact3(B::class)->inc()->printMe();


/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return B
 */
function fact4(string $cn) {
    switch($cn) {
    case A::class:
    case B::class:
    default:
        echo "passed $cn, create B\n";
        $obj = new B('');
        break;
        $obj->append();
    }

    $obj->printMe();
    return $obj;
}

fact4(A::class)->append()->printMe();
fact4(B::class)->append()->printMe();
fact4(Emptyn::class)->append()->printMe();

/**
 * @kphp-generic T
 * @param class-string<T> $cn
 * @return Emptyn
 */
function fact5(string $cn) {
    switch($cn) {
    default:
        echo "passed $cn, create Emptyn\n";
        $obj = new Emptyn();
    }
    return $obj;
}

fact5(A::class);
fact5(B::class);
fact5(Emptyn::class);


function ceNonGen() {
    switch ('kk') {
    case 'kk':
        $obj = new A(30);
        $obj->inc();
        $obj->printMe();
        break;
    case 'dd':
        // this case never reached, dropped off early and does not prevent compilation
        $obj = new B('ceNonGen');
        $obj->append2374623874();
        break;
    }
}

ceNonGen();

class BaseC {
    const TYPE = 'curator';

    static function info() {
        switch (static::TYPE) {
        case 'false':
        case 'curator':
        case 'D3':
            $t = static::TYPE;
            echo "overridden for $t in ", static::class, "\n";
            break;
        case 'D4':
            if(0) exit();
            if(1)
                var_dump(0);
            break;
        default:
            echo "const of ", static::class, "\n";
        }
    }
}
class D1 extends BaseC {}
class D2 extends BaseC { const TYPE = 'D2'; }
class D3 extends BaseC { const TYPE = 'D3'; }
class D4 extends BaseC { const TYPE = 'D4'; }

BaseC::info();
D1::info();
D2::info();
D3::info();
D4::info();


class ModelA {
    public int $a = 0;
}
class ModelB {
    public string $b = 'b';
}

function overloaded1(object $o) {
    echo "overloaded function for ", get_class($o), "\n";
    switch (classof($o)) {
    case ModelA::class:
        echo "a = ", $o->a;
        break;
    case ModelB::class:
        echo "b = '", $o->b, "'";
        break;
    }
    echo "\n";
}

overloaded1(new ModelA);
overloaded1(new ModelB);


/**
 * @kphp-generic T
 * @param T $o
 */
function overloaded2($o) {
    switch (true) {
    case true:
        switch (classof($o)) {
        case ModelA::class:
            if ($o === null) echo 'o = null';
            else echo "a = ", $o->a;
            break;
        default:
            break;
        }
    }
    echo "\n";
}

overloaded2(new ModelA);
overloaded2(new ModelB);
