@ok
<?php

var_dump(vk_sp_full_simplify( '') === '');
var_dump(vk_sp_full_simplify('ïðèâåò') === 'ïðèâåò');
var_dump(vk_sp_full_simplify('hello') === 'íållî');
var_dump(vk_sp_full_simplify('ïðèâåò hello 123') === 'ïðèâåòíållî12ç');
var_dump(vk_sp_full_simplify('<b>ïðèâåò</b>') === 'âïðèâåòâ');
var_dump(vk_sp_full_simplify('HELLO') === 'íållî');
var_dump(vk_sp_full_simplify('?') === '');

?>

--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
