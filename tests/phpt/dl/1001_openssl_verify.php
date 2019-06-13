@ok openssl
<?php

$certificate = <<<EOF
-----BEGIN CERTIFICATE-----
MIICBDCCAW0CAgPoMA0GCSqGSIb3DQEBCwUAMEoxCzAJBgNVBAYTAlVTMQowCAYD
VQQIDAFBMQowCAYDVQQHDAFCMQowCAYDVQQKDAFDMQswCQYDVQQLDAJWSzEKMAgG
A1UEAwwBRDAeFw0xODA3MjcxODA0NDJaFw0yODA3MjQxODA0NDJaMEoxCzAJBgNV
BAYTAlVTMQowCAYDVQQIDAFBMQowCAYDVQQHDAFCMQowCAYDVQQKDAFDMQswCQYD
VQQLDAJWSzEKMAgGA1UEAwwBRDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA
q7qq9FoSgwjLKP8PE5GsjbjIPoWelieznuEISnlTXKIcR7mHDkAv6blB5RmsE3hQ
jBMW1gAj7cAF5VIuwvRdICUPLh3yh4kmAJa6ApPltTKOCI7XbpiVN+nRPvUXQ+ox
Kruad/7Mt08YJ8/8G/SF+QU7IBtu3WR/W3JS6G/BFVECAwEAATANBgkqhkiG9w0B
AQsFAAOBgQBOlPLuQCHN0KbYgzcSsGjV3N1wBUINMHcrXNvOx3UUoDovFdRLNStH
nn4YPXbIdG3DqieFwDAlMYQ0EIJ/VRF01cHBlZDO4xkeN7ihpC4na4u3DdwCQZU0
bQahnsYaRtcCVgbSa/li/LrGEnn2wtt1Fu4y/inDb16O9WPppX1ZKA==
-----END CERTIFICATE-----
EOF;

$private_key = <<<EOF
-----BEGIN PRIVATE KEY-----
MIICdwIBADANBgkqhkiG9w0BAQEFAASCAmEwggJdAgEAAoGBAKu6qvRaEoMIyyj/
DxORrI24yD6FnpYns57hCEp5U1yiHEe5hw5AL+m5QeUZrBN4UIwTFtYAI+3ABeVS
LsL0XSAlDy4d8oeJJgCWugKT5bUyjgiO126YlTfp0T71F0PqMSq7mnf+zLdPGCfP
/Bv0hfkFOyAbbt1kf1tyUuhvwRVRAgMBAAECgYAelahMzJ3vaGmGa6arvY4Vz4sa
V4HfDEMZUMrBOMp3/Qc8XvaGuzfNUIlD3EahURRHXj767ht4BHMIXJKndg3/mOoD
8l/jXKE+fc5mB2TxM54jAzy7/fCc7eCcRxg3YN36jEFB/WRFdGKwDf2lDXRcz6K8
C5QazrbwiRfzlBAi4QJBANM7R3QXUNsyR53uJFNymK80cPFvl1jvqqt8wyXl6TXr
x0CVJILfUZUsBOXc1bQ9/yzOpuWIK2E57L0wfXWNglsCQQDQIB7kBpo+QbwmqQre
IBq/BZNfpfi0DYnEb/vnTt2TV4/S2Bhh/zts5CSnfy+MZqHZ1l9HcbJ6MlLO6rGZ
yn7DAkAweaosOi2UIDXPSJeNjv77Nk21GqbhAh9ou3kNeXXLqhBQAeofHqDaWv/E
wKlKc+/bmJE0m8tYxLJPuJESKSeBAkEAombeE+q/AyOCMNypWUjN9g7gv7sgBUen
H/yOISFeLvIdjVYIOLfT5BVmMLlDHhib5QKtWG906HtKOKHUMgBbxwJBALtWdc/X
vY+QcXG7b4AylV7KGExOJdKRRdMmaP6BNesTE5Q2RCeatSrIsXkwbb2ayUTGIGMC
OeYB4QO9EuluDVM=
-----END PRIVATE KEY-----
EOF
;

$public_key = <<<EOF
-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCruqr0WhKDCMso/w8TkayNuMg+
hZ6WJ7Oe4QhKeVNcohxHuYcOQC/puUHlGawTeFCMExbWACPtwAXlUi7C9F0gJQ8u
HfKHiSYAlroCk+W1Mo4IjtdumJU36dE+9RdD6jEqu5p3/sy3Txgnz/wb9IX5BTsg
G27dZH9bclLob8EVUQIDAQAB
-----END PUBLIC KEY-----
EOF;


$data = "Hello, world!";

$signature_sha1 = "";
$signature_md5 = "";

openssl_sign($data, $signature_sha1, $private_key);
openssl_sign($data, $signature_md5, $private_key, OPENSSL_ALGO_MD5);

var_dump(openssl_verify($data, $signature_sha1, $public_key));
var_dump(openssl_verify($data, $signature_md5, $public_key, OPENSSL_ALGO_MD5));

var_dump(openssl_verify($data, $signature_sha1, $public_key, OPENSSL_ALGO_MD5));
var_dump(openssl_verify($data, $signature_md5, $public_key));

//$signature_sha1[0] = 10;
//$signature_md5[0] = 10;
//var_dump(openssl_verify($data, $signature_sha1, $public_key));
//var_dump(openssl_verify($data, $signature_md5, $public_key, OPENSSL_ALGO_MD5));

