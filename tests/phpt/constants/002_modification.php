@kphp_should_fail
/Modification of constant: INT_CONST/
<?php
function getInt(int $x) {
    return ($x + 4) * 2;
}
const INT_CONST = getInt(17);
INT_CONST = 123;
