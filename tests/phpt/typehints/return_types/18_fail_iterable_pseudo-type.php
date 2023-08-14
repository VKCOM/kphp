@kphp_should_fail
\Iterable type isn't supported\
<?php
function foo(): iterable {
    return [1, 2, 3];
}
foo();
