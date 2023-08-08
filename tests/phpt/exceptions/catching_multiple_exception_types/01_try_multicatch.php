@kphp_should_fail
\Catching multiple exception types isn't supported\
<?php

class ExceptionType1 extends Exception {}
class ExceptionType2 extends Exception {}

try {
    // ...
} catch (ExceptionType1 | ExceptionType2 $e) {
    // ...
}
