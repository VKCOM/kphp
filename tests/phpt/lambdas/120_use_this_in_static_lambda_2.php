@kphp_should_fail
/Using \$this in a static lambda/
<?php

class A {
    public function bar() {
        echo "nar";
    }
    public function foo() {
        (function() {
            (static function() {
                (function() {
                    self::bar();
                });
            })();
        })();
    }
}

$a = new A();
$a->foo();
