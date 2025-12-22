<?php

function main() {
  $raw_post_data = file_get_contents('php://input');
  $res = prepare_search_query($raw_post_data);
  $resp = array("POST_BODY" => $res);
  echo json_encode($resp);
}

main();
