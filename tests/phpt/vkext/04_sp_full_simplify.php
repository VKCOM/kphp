@ok
<?php

var_dump(vk_sp_full_simplify( '') === '');
var_dump(vk_sp_full_simplify('������') === '������');
var_dump(vk_sp_full_simplify('hello') === '��ll�');
var_dump(vk_sp_full_simplify('������ hello 123') === '��������ll�12�');
var_dump(vk_sp_full_simplify('<b>������</b>') === '��������');
var_dump(vk_sp_full_simplify('HELLO') === '��ll�');
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
