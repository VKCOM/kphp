@kphp_should_fail
/default value of interface parameter:`1234:const int` may not differ from value of derived parameter: `56789:const int`, in function: AA::foo/
<?php

interface II {
    function foo($x = 1234);
}

class AA implements II {
    function foo($x = 56789) { var_dump($x); }
}

class BB implements II {
    function foo($x = 0) { var_dump($x); }
}

function foo(II $ii) { $ii->foo(); }

function demo() {
    foo(new AA);
    foo(new BB);
}

demo();
