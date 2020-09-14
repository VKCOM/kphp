@ok
<?php
require_once 'kphp_tester_include.php';

var_dump(Classes\TestFieldClassParent::getSelfClass());
var_dump(Classes\TestFieldClassParent::getStaticClass());

var_dump(Classes\TestFieldClass::getSelfClass());
var_dump(Classes\TestFieldClass::getStaticClass());
var_dump(Classes\TestFieldClass::getParentClass());

var_dump(Classes\TestFieldClass::class);
var_dump(Classes\TestFieldClassParent::class);

