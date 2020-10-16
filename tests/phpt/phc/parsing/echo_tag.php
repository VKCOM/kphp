@ok

<?php
  $ids = [1, 2, 3];
?>

<?php echo 1 ?>
<?= 2; ?>
<?= 3 ?>

<span id="<?= $ids[1] ?>"></span>

// only 4 is echoed. 5 is discarded
<?= 4; 5 ?>

// both 6 and 7 are printed
<?= 6; echo 7 ?>
