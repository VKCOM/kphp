@ok
<?php

var_dump(vk_sp_full_simplify( '') === '');
var_dump(vk_sp_full_simplify('привет') === 'привет');
var_dump(vk_sp_full_simplify('hello') === 'неllо');
var_dump(vk_sp_full_simplify('привет hello 123') === 'приветнеllо12з');
var_dump(vk_sp_full_simplify('<b>привет</b>') === 'вприветв');
var_dump(vk_sp_full_simplify('HELLO') === 'неllо');
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
