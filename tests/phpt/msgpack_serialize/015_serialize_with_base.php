@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class Base {
    public function foo() { var_dump("OK"); }
}

/** @kphp-serializable */
class A extends Base {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 10;

    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 10;
}

$a = new A();
$str = instance_serialize($a);
$a_new = instance_deserialize($str, A::class);
$a_new->foo();

$str_safe = instance_serialize_safe($a);
$a_new_safe = instance_deserialize_safe($str_safe, A::class);
$a_new_safe->foo();
var_dump($a_new_safe->y);

/** @kphp-serializable */
class B22 {
    /**
     * @kphp-serialized-field 1
     * @var int
     */
    public $x = 0;
}

/** @kphp-serializable */
class B21 {
    /**
     * @kphp-serialized-field 1
     * @var B22
     */
    public $x;

    public function __construct() {
        $this->x = new B22();
    }
}

/** @kphp-serializable */
class B20 {
    /**
     * @kphp-serialized-field 1
     * @var B21
     */
    public $x;

    public function __construct() {
        $this->x = new B21();
    }
}

/** @kphp-serializable */
class B19 {
    /**
     * @kphp-serialized-field 1
     * @var B20
     */
    public $x;

    public function __construct() {
        $this->x = new B20();
    }
}

/** @kphp-serializable */
class B18 {
    /**
     * @kphp-serialized-field 1
     * @var B19
     */
    public $x;

    public function __construct() {
        $this->x = new B19();
    }
}

/** @kphp-serializable */
class B17 {
    /**
     * @kphp-serialized-field 1
     * @var B18
     */
    public $x;

    public function __construct() {
        $this->x = new B18();
    }
}

/** @kphp-serializable */
class B16 {
    /**
     * @kphp-serialized-field 1
     * @var B17
     */
    public $x;

    public function __construct() {
        $this->x = new B17();
    }
}

/** @kphp-serializable */
class B15 {
    /**
     * @kphp-serialized-field 1
     * @var B16
     */
    public $x;

    public function __construct() {
        $this->x = new B16();
    }
}

/** @kphp-serializable */
class B14 {
    /**
     * @kphp-serialized-field 1
     * @var B15
     */
    public $x;

    public function __construct() {
        $this->x = new B15();
    }
}

/** @kphp-serializable */
class B13 {
    /**
     * @kphp-serialized-field 1
     * @var B14
     */
    public $x;

    public function __construct() {
        $this->x = new B14();
    }
}

/** @kphp-serializable */
class B12 {
    /**
     * @kphp-serialized-field 1
     * @var B13
     */
    public $x;

    public function __construct() {
        $this->x = new B13();
    }
}

/** @kphp-serializable */
class B11 {
    /**
     * @kphp-serialized-field 1
     * @var B12
     */
    public $x;

    public function __construct() {
        $this->x = new B12();
    }
}

/** @kphp-serializable */
class B10 {
    /**
     * @kphp-serialized-field 1
     * @var B11
     */
    public $x;

    public function __construct() {
        $this->x = new B11();
    }
}

/** @kphp-serializable */
class B9 {
    /**
     * @kphp-serialized-field 1
     * @var B10
     */
    public $x;

    public function __construct() {
        $this->x = new B10();
    }
}

/** @kphp-serializable */
class B8 {
    /**
     * @kphp-serialized-field 1
     * @var B9
     */
    public $x;

    public function __construct() {
        $this->x = new B9();
    }
}

/** @kphp-serializable */
class B7 {
    /**
     * @kphp-serialized-field 1
     * @var B8
     */
    public $x;

    public function __construct() {
        $this->x = new B8();
    }
}

/** @kphp-serializable */
class B6 {
    /**
     * @kphp-serialized-field 1
     * @var B7
     */
    public $x;

    public function __construct() {
        $this->x = new B7();
    }
}

/** @kphp-serializable */
class B5 {
    /**
     * @kphp-serialized-field 1
     * @var B6
     */
    public $x;

    public function __construct() {
        $this->x = new B6();
    }
}

/** @kphp-serializable */
class B4 {
    /**
     * @kphp-serialized-field 1
     * @var B5
     */
    public $x;

    public function __construct() {
        $this->x = new B5();
    }
}

/** @kphp-serializable */
class B3 {
    /**
     * @kphp-serialized-field 1
     * @var B4
     */
    public $x;

    public function __construct() {
        $this->x = new B4();
    }
}

/** @kphp-serializable */
class B2 {
    /**
     * @kphp-serialized-field 1
     * @var B3
     */
    public $x;

    public function __construct() {
        $this->x = new B3();
    }
}

/** @kphp-serializable */
class B1 {
    /**
     * @kphp-serialized-field 1
     * @var B2
     */
    public $x;

    public function __construct() {
        $this->x = new B2();
    }
}

try {
    instance_serialize_safe(new B1());
} catch (\Exception $e) {
    var_dump($e->getMessage());
}
