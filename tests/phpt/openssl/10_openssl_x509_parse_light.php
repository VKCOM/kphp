@ok
<?php

function print_cert_info(string $certificate) {
  $res = openssl_x509_parse($certificate);
  var_dump($res["name"]);
  var_dump($res["subject"]["C"]);
  var_dump($res["subject"]["ST"]);
  var_dump($res["subject"]["O"]);
  var_dump($res["subject"]["CN"]);
  var_dump($res["issuer"]["C"]);
  var_dump($res["issuer"]["ST"]);
  var_dump($res["issuer"]["O"]);
  var_dump($res["issuer"]["CN"]);
  var_dump($res["version"]);
  var_dump($res["validFrom_time_t"]);
  var_dump($res["validTo_time_t"]);

  // Currently not supported in K2
  // var_dump($res["hash"]);

  // Currently not supported in K2
  // var_dump($res["purposes"]);
}

$certificate = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDazCCAlOgAwIBAgIUGueadw4P4qBbd4ZcVxGKRdh2xaYwDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMDEwMjExMzI4MzhaFw0yMTEw
MjExMzI4MzhaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQCwVgkvzmAZkmMNCkcu1WTNFoFt352RM3TEuOsHZySR
Mk+5Mwc1lwokbKw3gjIN794xGNWcGHeg4iR7bOOPvmfNYgsqcnTHDCG1A3YJB9D+
JwPKV1Kk+23JfqLJ35RuFQzBF7YH+CVV5/1KiJhie6SVtF7EP4kKPMiH9j4Xs7IV
MyAN6kqdQNznNeDt4Q3L4nuWmsSNk+z5eZpRLG8A16uZXXWXnGyKqwutw09zMqdu
1kPy/fpJt8ftbj4uiP/He/cs2K4Us2kyYj30UA9jpvDvQcRBcU0AxA1DEO8jnX2W
3aY5R2KIvf8g+MYYBaDLkZr5Ro3+VooBtLHWFMRQhvQDAgMBAAGjUzBRMB0GA1Ud
DgQWBBSH0c+M4Eivckk1aqQwU34VWlg85zAfBgNVHSMEGDAWgBSH0c+M4Eivckk1
aqQwU34VWlg85zAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAb
1NzCrBanG9w+VB+lIBJpnsy8gpPcZrXvgqzXnRFRp7LZXwZjqyKxwxzB9ik6dTxg
ck1fqpArUwLnjI7qzBKQaZc/7hX4jOVPXSz+7Yef1gdGzQaGSv8+L/Q6Z1jCO94y
MT7L4T6XXPK034+9SkSN17OT1sKNIn30ZuIU2pJCyxESE96PQAfZ3/kSoI+N4Edx
8nXNmxIxur4vcIeIqfTyodQmyTWzqpMwzltermP9y12lWf9Oxwy07VPDqLfpeeC2
8ft2o8gj7Xa5BdWzdJ/JPBGXodQbT+OuYhH2E+GVb8xu1cb4Dr7hpy5Y+mwG0tR5
SwD3IhxiqzidJOkIFPyT
-----END CERTIFICATE-----
EOF;

print_cert_info($certificate);

$certificate = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDazCCAlOgAwIBAgIUGueadw4P4qBbd4ZcVxGKRdh2xaYwDQYJKoZIhvcNAQEL
BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM
GEludGVybmV0
MjExMzI4MzhaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw
HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQCwVgkvzmAZkmMNCkcu1WTNFoFt352RM3TEuOsHZySR
Mk+5Mwc1lwokbKw3gjIN794xGNWcGHeg4iR7bOOPvmfNYgsqcnTHDCG1A3YJB9D+
JwPKV1Kk+23JfqLJ35RuFQzBF7YH+CVV5/1KiJhie6SVtF7EP4kKPMiH9j4Xs7IV
MyAN6kqdQNznNeDt4Q3L4nuWmsSNk+z5eZpRLG8A16uZXXWXnGyKqwutw09zMqdu
1kPy/fpJt8ftbj4uiP/He/cs2K4Us2kyYj30UA9jpvDvQcRBcU0AxA1DEO8jnX2W
3aY5R2KIvf8g+MYYBaDLkZr5Ro3+VooBtLHWFMRQhvQDAgMBAAGjUzBRMB0GA1Ud
DgQWBBSH0c+M4Eivckk1aqQwU34VWlg85zAfBgNVHSMEGDAWgBSH0c+M4Eivckk1
aqQwU34VWlg85zAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAb
1NzCrBanG9w+VB+lIBJpnsy8gpPcZrXvgqzXnRFRp7LZXwZjqyKxwxzB9ik6dTxg
ck1fqpArUwLnjI7qzBKQaZc/7hX4jOVPXSz+7Yef1gdGzQaGSv8+L/Q6Z1jCO94y
MT7L4T6XXPK034+9SkSN17OT1sKNIn30ZuIU2pJCyxESE96PQAfZ3/kSoI+N4Edx
8nXNmxIxur4vcIeIqfTyodQmyTWzqpMwzltermP9y12lWf9Oxwy07VPDqLfpeeC2
8ft2o8gj7Xa5BdWzdJ/JPBGXodQbT+OuYhH2E+GVb8xu1cb4Dr7hpy5Y+mwG0tR5
SwD3IhxiqzidJOkIFPyT
-----END CERTIFICATE-----
EOF;

print_cert_info($certificate);
