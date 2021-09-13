@ok php8
<?php

function wrong() {
  return "wrong";
}

echo match (false) {
  '' => wrong(),
  [] => wrong(),
  0 => wrong(),
  0.0 => wrong(),
  false => "false\n",
};
