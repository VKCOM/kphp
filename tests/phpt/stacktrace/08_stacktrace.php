@kphp_should_fail
/pass SendResult to argument \$x of acceptInt/
/\$result is SendResult/
/Demo::send returns SendResult/
/Demo::send declared that returns SendResult/
<?php

class SendResult {
    function prin() { }
}

trait T {
    final protected static function send(): SendResult {
        return new SendResult;
    }

    final public static function update(): SendResult {
        $result = static::send();
        acceptInt($result);
        return $result;
    }
}

function acceptInt(int $x) {}

class Demo {
    use T;
}

Demo::update();

