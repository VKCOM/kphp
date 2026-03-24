@ok 
<?php

require_once 'kphp_tester_include.php';

$a = ' �������������������������������!�"�#�$�%�&�\'�(�)�*�+�,�-�.�/�0�1�2�3�4�5�6�7�8�9�:�;�<�=�>�?�@�A�B�C�D�E�F�G�H�I�J�K�L�M�N�O�P�Q�R�S�T�U�V�W�X�Y�Z�[�\�]�^�_�`�a�b�c�d�e�f�g�h�i�j�k�l�m�n�o�p�q�r�s�t�u�v�w�x�y�z�{�|�}�~��';
$serialized = msgpack_serialize($a);
var_dump(base64_encode($serialized));

$a_new = msgpack_deserialize($serialized);
var_dump($a === $a_new);
var_dump($a_new);

$a = 'here i am';
$serialized = msgpack_serialize($a);
var_dump(base64_encode($serialized));
$a_new = msgpack_deserialize($serialized);
var_dump($a === $a_new);
var_dump($a_new);
