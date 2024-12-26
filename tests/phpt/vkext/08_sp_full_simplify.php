@ok
<?php

var_dump(vk_sp_full_simplify('') === '');
var_dump(vk_sp_full_simplify('hello ©') === 'íållîñ');
var_dump(vk_sp_full_simplify('test 0346') === 'òåsòîç÷á');
var_dump(vk_sp_full_simplify('?') === '');

?>

--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
