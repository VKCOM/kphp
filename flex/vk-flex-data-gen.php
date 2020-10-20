<?php

/**
 * Файл, транслирующий флекс конфиг, зашитый в php коде в структуры c++ для vkext
 */

require_once "lib/vkext-flex-generate.lib.php";
require_once "lib/configs/flex-config.php";

global $langs, $tree, $vertex_num, $cases, $cases_num, $lang_num, $max_lang;

$langs = [];
$tree = [];
$vertex_num = 0;
$cases = [];
$cases_num = 0;
$lang_num = 0;
$max_lang = 0;

// Пишем дефинишны структур
print_includes();

// Подтягиваем конфиг
for ($lang_id = 0; $lang_id < 256; $lang_id++) {
  $language_config = setupFlexNew($lang_id, []);
  if ($language_config === []) {
    continue;
  }

  create_lang($lang_id, $language_config);
}


// Пишем информацию о всех доступных склонениях и всех доступных языка
print_cases();
print_langs();

echo "extern \"C\" void vk_flex_data_dummy(void) {}\n";
