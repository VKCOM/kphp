@ok
<?php

function get_anon_class() {
    return new class {};
}

function get_another_anon_class() {
    return new class {};
}

function test_anon_classes_equality() {
  echo (get_class(get_anon_class()) === get_class(get_anon_class()));
  echo (get_class(get_anon_class()) === get_class(get_another_anon_class()));
}

test_anon_classes_equality();
