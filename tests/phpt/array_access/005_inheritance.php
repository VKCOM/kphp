@ok
<?php
require_once 'kphp_tester_include.php';

$called = false;

class DoubleAppend extends Classes\LoggingLikeArray {
    /**
     * @param $offset mixed
     * @param $value mixed
     */
    public function offsetSet($offset, $value) {
        global $called;
        $called = true;
        echo "Call DoubleAppend::offsetSet\n";
        if (is_null($offset)) {
            $this->data[] = $value;
            $this->data[] = $value;
        } else {
            $this->data[$offset] = $value;
        }
    }
};

function test() {
    global $called;

    $obj = new DoubleAppend();
    $obj[] = 123;
    $obj[] = "abcd";
    $obj[123] = ["string", 0.125, [1, 2, 3]];

    foreach ($obj->keys() as $key) {
        var_dump($obj[$key]);
    }

    if ($obj instanceof Classes\LoggingLikeArray) {
        $obj->offsetSet(null, $obj[123]);
        var_dump($obj[123]);
    } else {
        die_if_failure(false, "false negative instanceof Classes\LoggingLikeArray");
    }

    if ($obj instanceof \ArrayAccess) {
        $obj->offsetSet(null, 0.5);
        var_dump($obj[126]);
    } else {
        die_if_failure(false, "false negative instanceof \ArrayAccess");
    }
}

test();
