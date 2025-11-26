<?php

function main() {
  echo prepare_search_query(file_get_contents('php://input'));
}

main();
