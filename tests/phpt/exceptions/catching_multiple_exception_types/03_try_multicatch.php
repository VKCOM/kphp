@kphp_should_fail
\Catching multiple exception types isn't supported\
<?php

class ExceptionType1 extends Exception {}
class ExceptionType2 extends Exception {}
class ExceptionType3 extends Exception {}

try {
    try {
        // ...
    } catch (ExceptionType1 | ExceptionType3 $e) {
        // ...
    }
} catch (ExceptionType2 | ExceptionType3 $e) {
    // ...
}
