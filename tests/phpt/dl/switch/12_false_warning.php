@ok
<?php  
define("TICKETS_AUTHOR_TYPE_USER", 0);

function checkTicketAccess($ticket) {
  $user_id = 1;

  switch ($ticket) {
    case TICKETS_AUTHOR_TYPE_USER:
    default:
      return $user_id == $ticket;
  }
  return false;
}

var_dump (checkTicketAccess (1));
