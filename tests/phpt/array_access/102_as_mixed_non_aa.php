@kphp_runtime_should_warn
/Class K doesn't implement \\ArrayAccess to be accessed, index = 0/
/Class K doesn't implement \\ArrayAccess to be accessed, index = 1/
/Class K doesn't implement \\ArrayAccess to be used in unset/
/Class K doesn't implement \\ArrayAccess to be used in isset/
/Class K doesn't implement \\ArrayAccess to be accessed, index = 4/

<?php
require_once 'kphp_tester_include.php';
class K {
    public int $x;
    public function __construct(int $y) {
        $this->x = $y;
    }
};

function test() {
    $obj = to_mixed(new K(2));
    $obj[0] = 1;

    var_dump($obj[1]);
    unset($obj[2]);
    var_dump(isset($obj[3]));
    var_dump(empty($obj[4]));
}

test();
