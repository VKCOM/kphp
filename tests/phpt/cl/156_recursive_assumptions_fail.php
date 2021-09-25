@kphp_should_fail
/\$idea is not an instance or it can't be detected/
<?php

// this can't be assumed of course, so "is not an instance" is expected
// the test checks that recursive assumptions don't hang

class Idea { function fa() {} }

function getAllIdeas() {
    $cache = null;

    if (1) {
      $idea = $cache;
    } else {
      $cache = $idea;
    }

    $idea->fa();
}

getAllIdeas();
