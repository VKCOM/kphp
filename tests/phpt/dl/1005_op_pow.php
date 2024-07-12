@ok k2_skip
<?php

define ('TWO', 2);

function test_integer() {
    /** @var int $x_int */
    /** @var mixed $x_var */
    /** @var mixed $y_var */

    echo $x_int = 3 ** TWO, "\n";
    echo $x_int = 3 ** 2, "\n";
    echo $x_int = -3 ** 2, "\n";
    echo $x_int = (-3) ** 2, "\n";
    echo $x_int = 2 ** 0x0F, "\n";
    echo $x_int = 0x10 ** 2, "\n";
    echo $x_int = 0x2 ** 0xB, "\n";
    echo $x_var = 2 ** -2, "\n";
    echo $x_var = 2 ** (-2), "\n";
    echo $x_var = -2 ** -2, "\n";
    echo $x_var = 2 ** 3 ** 2, "\n";
    echo $x_var = -2 ** 3 ** 2, "\n";
    echo $x_var = -2 ** 3 ** -2, "\n";
    echo $x_var = -2 ** -3 ** -2, "\n";
    echo $x_var = (2 ** 3) ** 2, "\n";
    echo $x_var = (-2 ** 3) ** -2, "\n";
    echo $x_var = 2 ** -(3 ** -2), "\n";
    echo $x_var = 3 ** (-2 ** 3) ** -2, "\n";

    $x_int = 2;
    $x_var = 2;
    echo $y_var = $x_int **= 2, "\n";
    echo $y_var = $x_var **= $x_int **= 0, "\n";
}

function test_double() {
    /** @var mixed $x_var */
    /** @var mixed $y_var */

    echo $x_var = 3.2 ** 2.1, "\n";
    echo $x_var = 3.1 ** -2.2, "\n";
    echo $x_var = -3.2 ** 2.0, "\n";
    echo $x_var = -3.1 ** -2.0, "\n";
    echo $x_var = -3.2 ** 2.0 ** 3.0, "\n";
    echo $x_var = -3.1 ** (2.0 ** -2.0), "\n";
    echo $x_var = (-3.1 ** 2.0) ** -2.0, "\n";
    echo $x_var = 4.5 ** (-3.1 ** 2.0) ** -2.0, "\n";

    $x_var = 2.2;
    echo $y_var = $x_var **= 0.5, "\n";
    echo $y_var = $x_var **= $x_var ** 1.5, "\n";
}

function test_integer_double() {
    /** @var mixed $x_var1 */
    /** @var mixed $x_var2 */
    /** @var mixed $x_var3 */
    /** @var mixed $x_var5 */
    /** @var mixed $y_var */

    echo $x_var1 = -3 ** 2.1, "\n";
    echo $x_var1 = -2.3 ** 2 ** 1.5, "\n";
    echo $x_float1 = -2.2 ** 2, "\n";
    echo $x_float1 = -2.2 ** 2 ** 3, "\n";

    /** @var float $x_float1 */

    $x_var2 = 2;
    $x_var3 = 5;
    $x_float1 = 2.2;
    $x_var5 = 0.5;

    echo $y_var = $x_var2 **= 0.5, "\n";
    echo $y_var = $x_var3 **= $x_var2 ** 1.5, "\n";

    echo $y_var = $x_float1 **= 2, "\n";
    echo $y_var = $x_var5 **= $x_float1 ** 2, "\n";
}

function test_prefix() {
    /** @var int $x_int */
    /** @var mixed $x_var */

    $x_int = 1;
    echo $x_var = 2 ** ++$x_int, "\n";
    echo $x_int = ++$x_int ** 2, "\n";

    echo $x_var = -2 ** +(--$x_int), "\n";
    echo $x_int = -(--$x_int) ** 2, "\n";
}

function test_postfix() {
    /** @var int $x_int */
    /** @var mixed $x_var */

    $x_int = 1;
    echo $x_int = $x_int++ ** 2, "\n";
    echo $x_var = 2 ** $x_int++, "\n";
}

function test_string() {
    /** @var mixed $x_var */

    echo $x_var = "3.1" ** "-2.3", "\n";
    echo $x_var = "-3" ** 2, "\n";
    echo $x_var = "-3" ** 2.0, "\n";
    echo $x_var = 2 ** "2", "\n";
    echo $x_var = 2.5 ** "2", "\n";
    echo $x_var = "-3" ** false, "\n";
    echo $x_var = false ** "2.2", "\n";

    echo $x_var = "bar" ** "foo", "\n";
}

function test_false() {
    /** @var mixed $x_var */

    if (1) echo $x_var = false ** 2;
    if (1) echo $x_var = 2 ** false;
    if (1) echo $x_var = 2.5 ** false;
    if (1) echo $x_var = false ** 2.5;
    
    if (1) echo $x_var = false ** false, "\n";
    if (1) echo $x_var = !false ** false, "\n";
    if (1) echo $x_var = false ** !false, "\n";
    if (1) echo $x_var = !false ** !false, "\n";
}

function test_or_false() {
    /** @var int|false $x1_int_or_false */
    /** @var int|false $x2_int_or_false */
    /** @var int $y_int */
    /** @var mixed $y_var */

    $x1_int_or_false = 1 ? 2 : false;
    echo $x1_int_or_false **= 2, "\n";

    $y_int = $x1_int_or_false ** 2;
    echo $y_int, "\n";

    $y_var = 2 ** $x1_int_or_false;
    echo $y_var, "\n";
    echo $y_var = 2.1 ** $x1_int_or_false, "\n";

    $x2_int_or_false = 0 ? 2 : false;
    echo $x2_int_or_false **= 2, "\n";
    echo $y_int = $x2_int_or_false ** 2, "\n";
    echo $y_var = 2 ** $x2_int_or_false, "\n";
    echo $y_var = 2.1 ** $x2_int_or_false, "\n";

    echo $y_var = $x1_int_or_false ** $x2_int_or_false, "\n";
    echo $x1_int_or_false **= 2, "\n";
    echo $x2_int_or_false **= 3, "\n";
}

function test_priority() {
    echo 2 ** 3 ** 2 + 2 ** 3 * 8 + 54 - 2 ** -7, "\n";
    echo 2 ** 3. ** false + "2" ** 3 * 8 + 54 - false ** -7, "\n";

    echo (int)2.5 ** 2, "\n";
    echo 2 ** (int)2.7, "\n";

    $x = 2;
    echo ++$x ** 2, "\n";
    echo $x++ ** 2, "\n";
    echo 2 ** ++$x, "\n";
    echo 2 ** $x++, "\n";

    echo (int)++$x ** 2, "\n";

    $x_arr1 = [];
    echo @$x_arr1 ** 2, "\n";

    $x_arr2 = [];
    echo @(int)$x_arr1 ** 2, "\n";
}

function test_array_arrow() {
    $x = array(1 => 2 ** 3.1, 2 ** 2 => 17, 2 ** 3 => 1.2 ** 5, );
    echo $x[1], "\n";
    echo $x[2**2], "\n";
    echo $x[2**3], "\n";
    echo $x[3**3], "\n";
}

test_integer();
test_double();
test_integer_double();
test_prefix();
test_postfix();
test_string();
test_false();
test_or_false();
test_priority();
test_array_arrow();
