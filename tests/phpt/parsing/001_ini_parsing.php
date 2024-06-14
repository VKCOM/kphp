@ok
<?php

$ini = '
[simple]
val_one="some value"
val_two=567

[vars]
int=12
float=124
b1=On
b2=OFF
b3=True
b4=false
str1=\'single quoted string\'
str2="double quoted string"
';

$ini_sections = '
[one]
[two]
[three]
';


function test_parse_ini_string_empty() {
    var_dump(parse_ini_string(""));
}

function test_parse_ini_string_sections() {
    var_dump(parse_ini_string("[one] [two] [three]", false));
    var_dump(parse_ini_string("[one] [two] [three]", true));
}

function test_parse_ini_string_scanner_normal($ini_string) {
    var_dump(parse_ini_string($ini_string, false, INI_SCANNER_NORMAL));
    var_dump(parse_ini_string($ini_string, true, INI_SCANNER_NORMAL));
}

function test_parse_ini_string_scanner_raw($ini_string) {
    var_dump(parse_ini_string($ini_string, false, INI_SCANNER_RAW));
    var_dump(parse_ini_string($ini_string, true, INI_SCANNER_RAW));
}

function test_parse_ini_string_scanner_typed($ini_string) {
    var_dump(parse_ini_string($ini_string, false, INI_SCANNER_TYPED));
    var_dump(parse_ini_string($ini_string, true, INI_SCANNER_TYPED));
}

function test_parse_ini_file_empty() {
    var_dump(parse_ini_file(""));
}

function test_parse_ini_file_sections($ini_sections) {
    file_put_contents("test.ini", $ini_sections);
    var_dump(parse_ini_file("test.ini", false));
    var_dump(parse_ini_file("test.ini", true));
}

function test_parse_ini_file_scanner_normal($ini_string) {
    file_put_contents("test.ini", $ini_string);
    var_dump(parse_ini_file("test.ini", false, INI_SCANNER_NORMAL));
    var_dump(parse_ini_file("test.ini", true, INI_SCANNER_NORMAL));
}

function test_parse_ini_file_scanner_raw($ini_string) {
    file_put_contents("test.ini", $ini_string);
    var_dump(parse_ini_file("test.ini", false, INI_SCANNER_RAW));
    var_dump(parse_ini_file("test.ini", true, INI_SCANNER_RAW));
}

function test_parse_ini_file_scanner_typed($ini_string) {
    file_put_contents("test.ini", $ini_string);
    var_dump(parse_ini_file("test.ini", false, INI_SCANNER_TYPED));
    var_dump(parse_ini_file("test.ini", true, INI_SCANNER_TYPED));
}

test_parse_ini_string_empty();
test_parse_ini_string_sections();
test_parse_ini_string_scanner_normal($ini);
test_parse_ini_string_scanner_raw($ini);
test_parse_ini_string_scanner_typed($ini);

//test_parse_ini_file_empty();
test_parse_ini_file_sections($ini_sections);
test_parse_ini_file_scanner_normal($ini);
test_parse_ini_file_scanner_raw($ini);
test_parse_ini_file_scanner_typed($ini);
