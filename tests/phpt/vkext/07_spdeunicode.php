@ok
<?php

var_dump(vk_sp_deunicode('') === '');
var_dump(vk_sp_deunicode('- "ïðèâåò, ýòî ÿ!"') === '- "ïðèâåò, ýòî ÿ!"');
var_dump(vk_sp_deunicode('&#65;&#66;&#67;') === 'ABC');
var_dump(vk_sp_deunicode('&#193;&#385;&#262;') === 'ABC');
var_dump(vk_sp_deunicode('&#45;&#32;&#34;&#104;&#101;&#108;&#108;&#111;&#44;&#32;&#105;&#116;&#39;&#115;&#32;&#109;&#101;&#33;&#34;') === ('- "hello, it\'s me!"'));
?>

--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)