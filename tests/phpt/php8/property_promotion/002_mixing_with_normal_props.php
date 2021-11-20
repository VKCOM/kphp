@ok php8
<?php

class Test {
    public string $prop2;

    public function __construct(public string $prop1 = "", $param2 = "") {
        $this->prop2 = $prop1 . $param2;
    }
}

$test = new Test("hello ", "world");
echo $test->prop1 . "\n";
echo $test->prop2 . "\n";
