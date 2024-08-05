@kphp_should_fail
\Iterable type isn't supported\
<?php
/**@return iterable*/
function foo() {
    return [1, 2, 3];
}
foo();
