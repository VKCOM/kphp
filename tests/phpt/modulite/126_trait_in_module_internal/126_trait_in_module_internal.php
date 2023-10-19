@kphp_should_fail
/restricted to use Feed126\\PrivTrait, it's internal in @feed/
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

class MMM126 {
    use Feed126\PrivTrait;
}
(new MMM126)->showThisClass();

// here $impl is auto-deduced as PrivClass because of typed callable
// so, we can call its instance methods, since we don't analyze instance methods
// but we can't write a type hint or extract this callback into another var, as we can't use that internal class in phpdoc
Feed126\PubFeed::accessToPrivClass(function($impl) {        // function(PrivClass $impl) is erroneous
    $impl->myMethod();
});
