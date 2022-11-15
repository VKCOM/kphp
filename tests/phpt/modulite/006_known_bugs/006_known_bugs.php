@ok
KPHP_ENABLE_MODULITE=1
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
#endif

// using internal trait works, because cloning trait methods works long before initing modulites in KPHP
class MMM006 {
    use Feed006\PrivTrait;
}
(new MMM006)->showThisClass();

// here $impl is auto-deduced as PrivClass because of typed callable
// so, we can call its instance methods, since we don't analyze instance methods
// but we can't write a type hint or extract this callback into another var, as we can't use that internal class in phpdoc
Feed006\PubFeed::accessToPrivClass(function($impl) {        // function(PrivClass $impl) is erroneous
    $impl->myMethod();
});
