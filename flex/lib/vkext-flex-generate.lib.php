<?php

/**
 * Библиотека для работы с трансляцией флекс конфигов в c++ код
 */

/**
 * Добавляет язык в глобальную переменную
 *
 * @param     $lang_id
 * @param int $z
 */
function add_lang($lang_id, $z = -1) {
  global $langs;
  global $max_lang;

  if ($z == -1) {
    $langs[$lang_id] = $lang_id;
  } else {
    $langs[$lang_id] = $z;
  }

  if ($lang_id > $max_lang) {
    $max_lang = $lang_id;
  }
}

function add_extra_langs() {
  $extra_lang_ids = array(11, 19, 52, 777, 888, 999);

  foreach ($extra_lang_ids as $lang_id) {
    add_lang($lang_id, 0);
  }
}

function new_vertex() {
  global $tree;
  global $vertex_num;

  $tree[$vertex_num++] = array("real" => 0, "children" => array(), "hyphen" => -1);
  return $vertex_num - 1;
}

function free_tree() {
  global $vertex_num;

  $vertex_num = 0;
  new_vertex();
  new_vertex();
}

function add_cases($rule) {
  global $cases;
  global $cases_num;

  if ($rule == "fixed") {
    return;
  }

  assert(is_array($rule));

  foreach ($rule as $case => $end) {
    if (!isset ($cases[$case])) {
      $cases[$case] = $cases_num;
      $cases[$cases_num++] = $case;
    }
  }
}

function add_tree($vertex, $pattern, $male_rule, $female_rule) {
  global $tree;

  add_cases($male_rule);
  add_cases($female_rule);

  $have_hyphen = 0;
  $have_bracket = 0;
  $l = strlen($pattern);
  $current_len = 0;
  $tail_len = -1;
  $have_asterisk = 0;

  for ($i = $l - 1; $i >= 0; $i--) {
    $c = $pattern[$i];
    if ($c == '*') {
      $have_asterisk = 1;
      break;
    }
    if ($c == '-') {
      if ($i == $l - 1) {
        $have_hyphen = 1;
      }
      continue;
    }
    if ($c == ')') {
      assert($have_bracket == 0 && $current_len == 0);
      $have_bracket = 1;
      continue;
    }
    if ($c == '(') {
      assert($have_bracket);
      $tail_len = $current_len;
      continue;
    }

    if (!isset ($tree[$vertex]["children"][$c])) {
      $tree[$vertex]["children"][$c] = new_vertex();
    }
    $vertex = $tree[$vertex]["children"][$c];
    $current_len++;
  }

  if ($have_asterisk == 0) {
    $c = chr(0);
    if (!isset ($tree[$vertex]["children"][$c])) {
      $tree[$vertex]["children"][$c] = new_vertex();
    }
    $vertex = $tree[$vertex]["children"][$c];
  }

  if ($have_hyphen == 1) {
    if ($tree[$vertex]["hyphen"] == -1) {
      $tree[$vertex]["hyphen"] = new_vertex();
    }
    $vertex = $tree[$vertex]["hyphen"];
  }

  if ($tree[$vertex]["real"] == 1) {
    print "//!!! duplicate rule for pattern $pattern (old pattern " . $tree[$vertex]["pattern"] . ")\n";
  }

  $tree[$vertex]["real"] = 1;
  $tree[$vertex]["tail_len"] = ($tail_len != -1 ? $tail_len : $current_len);
  $tree[$vertex]["pattern"] = $pattern;
  $tree[$vertex]["male_rule"] = $male_rule;
  $tree[$vertex]["female_rule"] = $female_rule;
  $tree[$vertex]["have_asterisk"] = $have_asterisk;
}

function print_children_array($lang_id) {
  global $tree;
  global $vertex_num;

  $common_vertex = $vertex_num;
  for ($i = 0; $i < $vertex_num; $i++) {
    if ($tree[$i]["hyphen"] >= 0) {
      $common_vertex--;
    }
  }

  print "static const int lang_{$lang_id}_children[" . (2 * ($common_vertex - 2)) . "] = {";

  $x = 0;
  for ($i = 0; $i < $vertex_num; $i++) {
    $tree[$i]["start_children"] = $x;
    for ($_c = 0; $_c < 256; $_c++) {
      $c = chr($_c);
      if (isset ($tree[$i]["children"][$c])) {
        if ($x != 0) {
          print ",";
        }
        print $_c . "," . $tree[$i]["children"][$c];
        $x++;
      }
    }
    $tree[$i]["end_children"] = $x;
  }

  assert($x == $common_vertex - 2);

  print "};\n";
}

function print_endings($rule) {
  global $cases;
  global $cases_num;

  assert(is_array($rule));

  $y = 0;
  for ($i = 0; $i < $cases_num; $i++) {
    if ($y != 0) {
      print ",";
    }

    if (isset ($rule[$cases[$i]])) {
      print "\"" . $rule[$cases[$i]] . "\"";
    } else {
      print "0";
    }

    $y++;
  }
}

function print_endings_array($lang_id) {
  global $tree;
  global $vertex_num;

  print "static const char *lang_{$lang_id}_endings[] = {";

  $x = 0;
  for ($i = 0; $i < $vertex_num; $i++) if ($tree[$i]["real"] == 1) {
    if ($tree[$i]["male_rule"] == "fixed") {
      $tree[$i]["male_rule_num"] = -1;
    } else {
      if ($x != 0) {
        print ",";
      }
      $tree[$i]["male_rule_num"] = $x++;
      print_endings($tree[$i]["male_rule"]);
    }
    if ($tree[$i]["female_rule"] == "fixed") {
      $tree[$i]["female_rule_num"] = -1;
    } else {
      if ($x != 0) {
        print ",";
      }
      $tree[$i]["female_rule_num"] = $x++;
      print_endings($tree[$i]["female_rule"]);
    }
  }

  print "};\n";
}

function print_nodes_array($lang_id) {
  global $tree;
  global $vertex_num;

  print "static node lang_{$lang_id}_nodes[{$vertex_num}] = {\n";

  for ($i = 0; $i < $vertex_num; $i++) {
    $v = $tree[$i];

    print "  {";

    if ($v["real"] == 1) {
      print "/** .tail_len =*/ " . $v["tail_len"] . ", ";
      print "/** .hyphen =*/ " . $v["hyphen"] . ", ";
      print "/** .male_endings =*/ " . $v["male_rule_num"] . ", ";
      print "/** .female_endings =*/ " . $v["female_rule_num"] . ", ";
    } else {
      print "/** .tail_len =*/ -1, ";
      print "/** .hyphen =*/ " . $v["hyphen"] . ", ";
      print "/** .male_endings =*/ 0, ";
      print "/** .female_endings =*/ 0, ";
    }

    print "/** .children_start =*/ " . $v["start_children"] . ", ";
    print "/** .children_end =*/ " . $v["end_children"];

    if ($i != $vertex_num - 1) {
      print "},\n";
    } else {
      print "}\n";
    }
  }

  print "  };\n\n";
}

function recursive_fix_encoding(&$res) {
  if (is_array($res)) {
    foreach ($res as &$value) {
      recursive_fix_encoding($value);
    }
  } else if (is_string($res)) {
    $res = iconv("utf-8", "windows-1251", $res);
  }
}

function create_lang($lang_id, $res) {
  global $cases_num;

  recursive_fix_encoding($res);

  add_lang($lang_id);
  free_tree();

  $w_n = 0;
  if (isset ($res["names"])) {
    $w_n = 1;
    foreach ($res["names"] as $rule) {
      foreach ($rule["patterns"] as $pattern) {
        add_tree(0, $pattern, $rule["male"], $rule["female"]);
      }
    }
  }

  $w_s = 0;
  if (isset ($res["surnames"])) {
    $w_s = 1;
    foreach ($res["surnames"] as $rule) {
      foreach ($rule["patterns"] as $pattern) {
        add_tree(1, $pattern, $rule["male"], $rule["female"]);
      }
    }
  }

  print_children_array($lang_id);
  print_endings_array($lang_id);
  print_nodes_array($lang_id);

  print "static lang lang_{$lang_id} = {\n";
  print "  \"" . $res["flexible_symbols"] . "\",\n";

  if ($w_n == 1) {
    print "  /** .names_start =*/ 0,\n";
  } else {
    print "  /** .names_start =*/ -1,\n";
  }
  if ($w_s == 1) {
    print "  /** .surnames_start =*/ 1,\n";
  } else {
    print "  /** .surnames_start =*/ -1,\n";
  }

  print "  /** .cases_num =*/ {$cases_num},\n";
  print "  /** .children =*/ lang_{$lang_id}_children,\n";
  print "  /** .endings =*/ lang_{$lang_id}_endings,\n";
  print "  /** .nodes =*/ lang_{$lang_id}_nodes\n";
  print "};\n\n\n";
}

function print_cases() {
  global $cases;
  global $cases_num;

  print "const int CASES_NUM = $cases_num;\n";
  print "const char *cases_names[] = {";

  for ($i = 0; $i < $cases_num; $i++) {
    if ($i != 0) {
      print ",";
    }
    print "\"" . $cases[$i] . "\"";
  }

  print "};\n\n";
}

function print_langs() {
  global $langs;
  global $max_lang;

  add_extra_langs();

  $total_langs = $max_lang + 1;
  print "const int LANG_NUM = $total_langs;\n";
  print "lang *langs[] = {";
  for ($i = 0; $i < $total_langs; $i++) {
    if ($i > 0) {
      print ",";
    }
    if (isset ($langs[$i])) {
      print "&lang_" . $langs[$i];
    } else {
      print "0";
    }
  }
  print "};\n";
}

function print_includes() {
  print "#include \"flex/vk-flex-data.h\"\n";
}
