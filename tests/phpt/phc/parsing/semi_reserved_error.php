@kphp_should_fail
/Expected identifier, found 'else'/
/Expected identifier, found 'elseif'/
/A class constant must not be called 'class'; it is reserved for class name fetching/
<?php

function else() {}

const elseif = 10;

class ExampleConsts {
    const class = 'illegal';
}
