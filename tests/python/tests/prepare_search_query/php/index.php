<?php

function main() {
  $raw_post_data = file_get_contents('php://input');
  $post_data = json_decode($raw_post_data, $associative=true);
  $res = prepare_search_query($post_data["post"]);
  $resp = array("POST_BODY" => $res);
  echo json_encode($resp);
}

main();
