<?php

use VK\TL;

function send($q, $actor_id) {
$conn = new_rpc_connection('127.0.0.1', 2401, $actor_id);
$query_id = typed_rpc_tl_query_one($conn, $q);
return typed_rpc_tl_query_result_one($query_id);
}

function q1() {
  $q = new VK\TL\Functions\messages\getChatInfo;
  $q->user_id = 336098765;
  $q->peer_id = 2000000001;
  $q->fields_mask = 255;
  //$q->flags_and_mask = 0;
  $result = VK\TL\Functions\messages\getChatInfo::result(send($q, 14398));
  echo get_class($result), "\n";
  //var_dump(instance_to_array($result));

  new VK\TL\Types\messages\keyboardButtonColorPrimary;
  new VK\TL\Types\messages\keyboardButtonColorSecondary;
}

function q2() {
  $q = new VK\TL\Functions\messages\sublistMessages;
  $q->user_id = 336098765;
  $q->sublist = new VK\TL\Types\messages\sublistType(0,0);
  $q->from = 103713988;
  $q->to = 123;
  $q->fields_mask = 255;
  $q->max_len = 2;
  //$q->flags_and_mask = 0;
  $result = VK\TL\Functions\messages\sublistMessages::result(send($q, 14398));
  echo get_class($result), "\n";
  //var_dump(instance_to_array($result));
}

new \VK\TL\Types\rpcResponseOk;
new \VK\TL\Types\rpcResponseHeader;
new \VK\TL\Types\rpcResponseError;

q1();
q2();


$search_q = array (
              0 => 'search2.search',
              1 => 35287,
              2 =>
              array (
                'limit' => 10,
                'text' => 'univer',
                'tags' =>
                array (
                  0 => 'fA',
                ),
                'restrictions' =>
                array (
                  'lower_bounds' =>
                  array (
                    0 =>
                    array (
                      0 => 8,
                      1 => 1566345600,
                    ),
                    1 =>
                    array (
                      0 => 2,
                      1 => 10,
                    ),
                  ),
                  'upper_bounds' =>
                  array (
                  ),
                ),
                'ext_rating' =>
                array (
                  0 => 'expr.divide',
                  1 =>
                  array (
                    0 => 'expr.product',
                    1 =>
                    array (
                      0 => 'expr.ln',
                      1 =>
                      array (
                        0 => 'expr.addition',
                        1 =>
                        array (
                          0 => 'search2.exprRating',
                          1 => 2,
                        ),
                        2 =>
                        array (
                          0 => 'expr.int',
                          1 => 1,
                        ),
                      ),
                    ),
                    2 =>
                    array (
                      0 => 'expr.divide',
                      1 =>
                      array (
                        0 => 'expr.addition',
                        1 =>
                        array (
                          0 => 'expr.int',
                          1 => 1,
                        ),
                        2 =>
                        array (
                          0 => 'expr.product',
                          1 =>
                          array (
                            0 => 'expr.double',
                            1 => 0.25,
                          ),
                          2 =>
                          array (
                            0 => 'search2.exprOptionalTag',
                            1 => 'oA',
                            2 => 10.0,
                          ),
                        ),
                      ),
                      2 =>
                      array (
                        0 => 'expr.double',
                        1 => 3.5,
                      ),
                    ),
                  ),
                  2 =>
                  array (
                    0 => 'expr.pow',
                    1 =>
                    array (
                      0 => 'expr.if',
                      1 =>
                      array (
                        0 => 'expr.equal',
                        1 =>
                        array (
                          0 => 'search2.exprRating',
                          1 => 9,
                        ),
                        2 =>
                        array (
                          0 => 'expr.int',
                          1 => 0,
                        ),
                      ),
                      2 =>
                      array (
                        0 => 'expr.int',
                        1 => 4,
                      ),
                      3 =>
                      array (
                        0 => 'search2.exprRating',
                        1 => 9,
                      ),
                    ),
                    2 =>
                    array (
                      0 => 'expr.double',
                      1 => 0.075,
                    ),
                  ),
                ),
              ),
            );

function sendUn($q) {
  $conn = new_rpc_connection('127.0.0.1', 2401, 14511);
  $q_id = rpc_tl_query_one($conn, $q);
  $resp = rpc_tl_query_result_one($q_id);
  return $resp;
}

//var_dump(sendUn($search_q)['result']);

function q3() {

$q = new TL\Functions\search2\search();
$q->fields_mask = 35287;
echo "mask ", $q->fields_mask, "\n";
$q->query = new TL\Types\search2\searchQuery;
$q->query->limit = 10;
$q->query->text = 'univer';
$q->query->tags = ['fA'];
$q->query->restrictions = new TL\Types\search2\restrictions([[8, 1566345600], [2, 10]], []);

$q->query->ext_rating = new TL\Types\expr\divide(
  new TL\Types\expr\product(
    new TL\Types\expr\ln(
      new TL\Types\expr\addition(
        new TL\Types\search2\exprRating(2),
        new TL\Types\expr\int_(1)
      )
    ),
    new TL\Types\expr\divide(
      new TL\Types\expr\addition(
        new TL\Types\expr\int_(1),
        new TL\Types\expr\product(
          new TL\Types\expr\double_(0.25),
          new TL\Types\search2\exprOptionalTag('oA', 10.0)
        )
      ),
      new TL\Types\expr\double_(3.5)
    )
  ),
  new TL\Types\expr\pow(
    new TL\Types\expr\if_(
      new TL\Types\expr\equal(
        new TL\Types\search2\exprRating(9),
        new TL\Types\expr\int_(0)
      ),
      new TL\Types\expr\int_(4),
      new TL\Types\search2\exprRating(9)
    ),
    new TL\Types\expr\double_(0.075)
  )
);
  $conn = new_rpc_connection('127.0.0.1', 2401, 14511);
  $q_id = typed_rpc_tl_query_one($conn, $q);
  $resp = typed_rpc_tl_query_result_one($q_id);
  $result = TL\Functions\search2\search::result($resp);
  var_dump(get_class($result));
  var_dump(instance_to_array($result));
}

q3();

