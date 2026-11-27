@ok
<?php

$cert_ca_root = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDvDCCAqSgAwIBAgIBATANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjAwMTAxMDAwMDAw
WhcNMzUwMTAxMDAwMDAwWjCBjjELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlm
b3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAWBgNVBAoMD0V4YW1wbGUg
Um9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0aG9yaXR5MRgwFgYDVQQD
DA9FeGFtcGxlIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB
AQCyvzvXoOe04/Jndj64xVNgLUcyMoigBtdxKQaCezBGLQ98Y9CEQoie8Gy5PQTI
2NYGk5jWAwER5stvJuHqscrDsrgZfaEKmcBaAzRImZ9ImSKW0qGHVCnJTnqPNb1g
ufxY0xy8QSSS1tno2/AHcKR5sL0mEewSOe2qmPdxqaSDQXZJCS/0Tm6KMzmGZaga
LKZWY3L51L8KR+PXSTuFVtOVQaBIes0YAMbyzsHtzYiJBIFDnbLMhkF6lcfVzb2/
6jIWeBgyHSYYqDd69Hr0DFHe1GzcocpXDRoQrikzHFxnjBzkfcTdhqn0fD24JEIV
Ch7jRNod5ThpGoW68B1ljsWnAgMBAAGjIzAhMA8GA1UdEwEB/wQFMAMBAf8wDgYD
VR0PAQH/BAQDAgEGMA0GCSqGSIb3DQEBCwUAA4IBAQBkIiK01nF8qR+9kDW6b7r4
1dV1+9zZN6UV083nzkmHEzk228V4ay9xi6LbgSb1uTq+fL+Kuztv7wku0zhzI/5h
PgZdDv5mlUH8BBKFwKmt84SFGyZeJJYj+8kHjBve/Fqi8cnINWx/PtlCipq4viah
CPlyuZ79uPmqk6sHjt1tvxNAopeLbdo61IGd99SYil1aqPUOUgyUnFpxAecSsOQP
bAVSprrMVR12fqeERzQqmz9RFy7RwPADpB7VFC07v9Y/IDpSOf5Gc2dMj0HXgUBX
mvgDPxotOuML+hiNIYNvpAS7Zp3FqJbhcFS+H3AW0i1nkXPoJ/Wdy47lfd8h954N
-----END CERTIFICATE-----
EOF;

$cert_ca_pathlen = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBkTCCATegAwIBAgIBAjAKBggqhkjOPQQDAjA+MQswCQYDVQQGEwJERTEVMBMG
A1UECgwMSW50ZXJtZWRpYXRlMRgwFgYDVQQDDA9JbnRlcm1lZGlhdGUgQ0EwHhcN
MjEwMzA0MDUwNjA3WhcNMzEwMzA0MDUwNjA3WjA+MQswCQYDVQQGEwJERTEVMBMG
A1UECgwMSW50ZXJtZWRpYXRlMRgwFgYDVQQDDA9JbnRlcm1lZGlhdGUgQ0EwWTAT
BgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtqilDpK77TLb8U+nlkMqyZvMSp
vnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD65QyvmwjNsTpoyYwJDASBgNV
HRMBAf8ECDAGAQH/AgEAMA4GA1UdDwEB/wQEAwIBBjAKBggqhkjOPQQDAgNIADBF
AiEAqVHq6Ts5iZzW5J+wOfgAzUfBACeoJRqC8B7HDMQd51cCIG20nv6Qs+Rll7Oj
1ZV+w7Rojgrpk4HwKMcHv2sCZxRA
-----END CERTIFICATE-----
EOF;

$cert_ca_same_subject = <<<EOF
-----BEGIN CERTIFICATE-----
MIICHzCCAcagAwIBAgIBAzAKBggqhkjOPQQDAjCBjjELMAkGA1UEBhMCVVMxEzAR
BgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAWBgNV
BAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0aG9y
aXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjIwMjAyMDAwMDAwWhcN
MzIwMjAyMDAwMDAwWjCBjjELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3Ju
aWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAWBgNVBAoMD0V4YW1wbGUgUm9v
dCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0aG9yaXR5MRgwFgYDVQQDDA9F
eGFtcGxlIFJvb3QgQ0EwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQHgGzBIKwl
LmTuxmzZ8T1DUDJx51puN9cZwC0MyzprtKO0ZEeCp/yxnfrljiGKbKWMgt3qBnm9
CFyLAW4oBke1oxMwETAPBgNVHRMBAf8EBTADAQH/MAoGCCqGSM49BAMCA0cAMEQC
IG9YZqQbzJMM3h890vloGWx4/kxihqDa2ybP1vpQCb1eAiBJg/unEbi2Tqwe6rvg
GUcs7umiLT3kUIfy29mcqvuT0g==
-----END CERTIFICATE-----
EOF;

$cert_server = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDujCCAqKgAwIBAgIBZDANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjBSMQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZv
cm5pYTEUMBIGA1UECgwLRXhhbXBsZSBJbmMxGDAWBgNVBAMMD3d3dy5leGFtcGxl
LmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKXmxFn6T1OFRq78
yqWG10DLDp3h9OZJHjsk9Lg3wJwHy/Vhpbiwu4Bb3RB7a8yzvjn9BQczeFbRm+Sr
XXwovHabTEtuCsjn5EKJzrLmcR2EE7p8t1zDsLTtxOT63cxQMnc9i/Yg1j5H2Fti
1Cw44WIzf1uTT8qh47PLQCgJzlgZnzZb2TBGSRg4RMPJ6jHp7sZrM6z/4RpEidHY
S2oxgepdWhNz/tvaX2ogi1tfnTXAjjkyfumlfGsCNqYmJd6h48ER/b5HHSQO/mly
bQjVk0v5dxjoU1+q9Vf4U+xVCAq6afEnQKuR3rVxdnwMtZqbqy4NFRLGzM+N1iMw
MaeupHUCAwEAAaNeMFwwDAYDVR0TAQH/BAIwADAOBgNVHQ8BAf8EBAMCBaAwEwYD
VR0lBAwwCgYIKwYBBQUHAwEwJwYDVR0RBCAwHoIPd3d3LmV4YW1wbGUuY29tggtl
eGFtcGxlLmNvbTANBgkqhkiG9w0BAQsFAAOCAQEAD20jhdmPjkfKSRCrp0M3CzM3
r992loTsMrFievL3AatWrsjE6b7ouoiDpbA+lSg3lPkjjV1kwTdXf1klCoGM/eD+
w9kCxBHiU0cTw6+THKxw+6/GeA7Zan5BC81nxE38Cp1/RRJa8FUrvFNQjghwVeyg
taCglA+oMudN8qC7qN57jchxUoTdz+dr+wGRBRhS9s9kwROgm2sZo8NaVhCZObqG
13IHWQkj9fhV0HSJVH8lnrtT8wBIYgJ+2AWfIOEsVtzfhqlmOpNkJxZ4+CFiy9s5
YXxy0lsNbPJG1wrBe+uhMaBxPuPoESJayjZJS1jHJXGtRQiUA20N+wrle/4AJQ==
-----END CERTIFICATE-----
EOF;

$cert_client = <<<EOF
-----BEGIN CERTIFICATE-----
MIIClzCCAX+gAwIBAgIBZTANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjApMRQwEgYDVQQKDAtFeGFtcGxlIEluYzERMA8GA1UE
AwwIY2xpZW50LTEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtqilDp
K77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD65Qy
vmwjNsTpoy8wLTAJBgNVHRMEAjAAMAsGA1UdDwQEAwIHgDATBgNVHSUEDDAKBggr
BgEFBQcDAjANBgkqhkiG9w0BAQsFAAOCAQEAMt4Ti6qU5sevbTE6Cb1xWWlfykoA
BoCcAn5Etkfs8b8BupmRCZvVj9UwgurECSf89AquurgfHM2/bxLHq1dGQOvNSFKK
stRgpvtP3myi3kEqIodowUtLvBU9dy+w36HeL56mMrk7P4fopeRryvbhklK5q+iM
wZgYtAlVXLXVw8XNjYWdxvsdOgZA462/WiahcSdPuiPsD7otzif45pqf3HQQ8m83
JxNhuaTDINTIgjYIPxLqP3khq9pdOt0/fdy4duSjJnGnGwNZunsz8nKThir3ZX1g
ZuVuIlBnEQZfBTJ25E9rh5vMXc86ryPDbYuCYOpnPihF5dCnUFOLCX6vGg==
-----END CERTIFICATE-----
EOF;

$cert_smime = <<<EOF
-----BEGIN CERTIFICATE-----
MIICnTCCAYWgAwIBAgIBZjANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjA6MRYwFAYDVQQDDA1BbGljZSBFeGFtcGxlMSAwHgYJ
KoZIhvcNAQkBFhFhbGljZUBleGFtcGxlLmNvbTBZMBMGByqGSM49AgEGCCqGSM49
AwEHA0IABLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8xKm+c+W4lC+IlfRh+9bHaFY2
dE5HS2GVrCHpo38kpsPrlDK+bCM2xOmjJDAiMAsGA1UdDwQEAwIFoDATBgNVHSUE
DDAKBggrBgEFBQcDBDANBgkqhkiG9w0BAQsFAAOCAQEAroqViD6GoNhS15otkWV+
wUVQiD64Lx2U0Y6uyGPc6wDxlw8M2voUhIWdwMXtYRQ+TFk6GgsEnxBj/3h+dts+
VO94DDb5NFAkzw4bTaEQoqqyFBudHDmJrFSvW4K7nmR8JQEuD5f6M7PVxwbypSuA
FaesvxiC1lKzc1xDolCK5IPu2vg2d5Gh37MEjOMKGo2mUgeYMRyCAM+ntWCQUJee
Ol+pVQGyXas8XXqSOg5DGHFEihmARZZHilNvP/56STiZxGYpIhQT8RwYLjvKNUGA
eWqzLWkAKH/7sNf7LDWfEa9Rsvi3zycYjwaJE8EHoUuDT6kYCuw07FQf2Qw2RAk5
oA==
-----END CERTIFICATE-----
EOF;

$cert_code_signing = <<<EOF
-----BEGIN CERTIFICATE-----
MIICkjCCAXqgAwIBAgIBZzANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjAsMRQwEgYDVQQKDAtFeGFtcGxlIEluYzEUMBIGA1UE
AwwLQ29kZSBTaWduZXIwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtq
ilDpK77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD
65QyvmwjNsTpoycwJTAOBgNVHQ8BAf8EBAMCB4AwEwYDVR0lBAwwCgYIKwYBBQUH
AwMwDQYJKoZIhvcNAQELBQADggEBAKdjUDt7qsFHf+KqXg7LFvk4aDiqtGUHwqK4
/wX4LY9JdUuD7yh8sfkmp9RyvGfVlcvv1k1/ELYqA2rHO5GdzAkylTJfaAMDI0QM
QyLJKGWwQk4iq3Umy1NAg9IKEji7zunFVtKNneJ7ajG4KgnLxcN5Qo5lnm21gk4B
iT+uhANQNz9KobiISO5C0W5idBqhwy6cGHSsULabNIEgvA4B3G0d7WBPRCQquq2J
OIEmGYI6f3B7AcqiTl3kN6H1iKZM1164euRQnAlwop/4BTd7jeQfKqIZmoS8nQP1
d9e+P03pOeklNHlVwGndZJVDKFUv/+oxfd/7UyytntYCwHyjswg=
-----END CERTIFICATE-----
EOF;

$cert_all_eku = <<<EOF
-----BEGIN CERTIFICATE-----
MIICnjCCAYagAwIBAgIBaDANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjAWMRQwEgYDVQQDDAtBbnkgUHVycG9zZTBZMBMGByqG
SM49AgEGCCqGSM49AwEHA0IABLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8xKm+c+W4
lC+IlfRh+9bHaFY2dE5HS2GVrCHpo38kpsPrlDK+bCM2xOmjSTBHMEUGA1UdJQQ+
MDwGCCsGAQUFBwMBBggrBgEFBQcDAgYIKwYBBQUHAwQGCCsGAQUFBwMDBggrBgEF
BQcDCQYIKwYBBQUHAwgwDQYJKoZIhvcNAQELBQADggEBACuQC3k+V8lY4ua/gCbZ
3VoLITJQZF7amA4ufSkydF57ciN7p8cVZtxAeTG7fv0MI8q0q2BUopjb6wBeu5Tm
6tCj0BNjTeQn7R1hnrD1b9Y20BhMoUNvvkIAWP6hYlAxvheWXtZr+vv73lLZzV+4
9nKZrI2rpOpaAahEBp4PWwWyTqCBdryFxCbGevKlIOlE+SuaVb+SwnIy5LT4Kcl3
aRI7Cgi6jifzSvZkHao+TrkldR1vlXEvhR48rzkIY6qwKkEl812HtNxSl5GSKSdN
FgFwVlV0JTBRm5zmEkBs2fMUX45mEDXl02OHzhPtZSu7zAdTUrNfNIR5LSi2r7WQ
T7o=
-----END CERTIFICATE-----
EOF;

$cert_ocsp_signer = <<<EOF
-----BEGIN CERTIFICATE-----
MIICbzCCAVegAwIBAgIBaTANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjAZMRcwFQYDVQQDDA5PQ1NQIFJlc3BvbmRlcjBZMBMG
ByqGSM49AgEGCCqGSM49AwEHA0IABLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8xKm+
c+W4lC+IlfRh+9bHaFY2dE5HS2GVrCHpo38kpsPrlDK+bCM2xOmjFzAVMBMGA1Ud
JQQMMAoGCCsGAQUFBwMJMA0GCSqGSIb3DQEBCwUAA4IBAQAUqm8wbnvHzsx4ERkA
rKm5D7vdB7TDWEqobRcsz8LclIbR+jQvEc8g7+BCrP3xtGztF6YAx/cnQ8bcMdZv
BPpdfRYIvYUBEYHt1xRpU+xyb1XxuKlMVMO7ISVSkNKXQRhglizhzliu43oHvin7
3CLPrl/yqbZkoXXnKRuf8oHM0616tHKt/n+P/QbLxsRjMZv1zz9i9uf2uvGZkPcv
jMwnwrtQZ5assJvlItSebNFssXmTp4RIYw7JlwUqkTtFzQMoMlzGM6+F5Wp1Vcc8
PdXloNytOakABngdOJ6pTV2uBcMBdfu2AQPNqqSYijEV6jEB3n00YEwZaCtXoaw2
2VdD
-----END CERTIFICATE-----
EOF;

$cert_key_agreement_only = <<<EOF
-----BEGIN CERTIFICATE-----
MIICYzCCAUugAwIBAgIBajANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjASMRAwDgYDVQQDDAdESCBPbmx5MFkwEwYHKoZIzj0C
AQYIKoZIzj0DAQcDQgAEu/PeNN8LaopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV
9GH71sdoVjZ0TkdLYZWsIemjfySmw+uUMr5sIzbE6aMSMBAwDgYDVR0PAQH/BAQD
AgMIMA0GCSqGSIb3DQEBCwUAA4IBAQBct0214mAYrfjC9npZk+2RyTNZKMYJU3e7
TRJodD44gYZynfQcqCqb4aelxuVnvmikg6pE9ZHp6LHSJ4/kOZ2rPb4vgoZz9sHi
fkkkeZjrxtreiGeLdsnR1EJ68tBhZB6PwV2cQTgZ+uB2x4OXBom1KAgOwyrJe7Aw
gYeWpImbY9DwN32Y2vsi/O9Z2PJUwmD1KOGYbb7OGA1WYkhk+aEdYgq33To+oPXR
NOHL5EA7D009WElj1yfcoKhB2QWvxdpNQBweOZVJU+5lFFU60uCV8jSAFyEjSHU3
Jrjf6RwzKMaBAS0eoTHCv4QYKXLfkqqDfMlHm/TGuYHCcMPn0k5T
-----END CERTIFICATE-----
EOF;

$cert_crl_signer = <<<EOF
-----BEGIN CERTIFICATE-----
MIICZjCCAU6gAwIBAgIBazANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjAVMRMwEQYDVQQDDApDUkwgU2lnbmVyMFkwEwYHKoZI
zj0CAQYIKoZIzj0DAQcDQgAEu/PeNN8LaopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biU
L4iV9GH71sdoVjZ0TkdLYZWsIemjfySmw+uUMr5sIzbE6aMSMBAwDgYDVR0PAQH/
BAQDAgECMA0GCSqGSIb3DQEBCwUAA4IBAQCdYwbE9Glown9E90x756irkN+3cmE2
d6HajHY9VjsJ4ijOStQZhT7tffZmnDHR4SBslP5XB3n3rNdLMXnjcVVPrV61GgP1
Q+3yYEPriLnOGQaNv7cPbK01Jbio07kjT/b9ZqsiaevHsgZ6l9KFJH4pidfN9KDN
rGlYYDX5U6UpsBp27Kj+PfOQW9JcFUlF8STGywTXyNUXZaIEypCB+THQSPrSucd4
M/uVkhzCFKbL7B1uvaMDdH2NGgCIcXcstBgcH26B7Qxejf9Hx8HIfa1XCpGodw37
kxB5TZICld9YoDIXEGpssUOjJj/AnqDO15PLUXaVEOQ2I3s7m+O+yyEO
-----END CERTIFICATE-----
EOF;

$cert_ns_client = <<<EOF
-----BEGIN CERTIFICATE-----
MIICaDCCAVCgAwIBAgIBbDANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjAUMRIwEAYDVQQDDAlOUyBDbGllbnQwWTATBgcqhkjO
PQIBBggqhkjOPQMBBwNCAAS789403wtqilDpK77TLb8U+nlkMqyZvMSpvnPluJQv
iJX0YfvWx2hWNnROR0thlawh6aN/JKbD65QyvmwjNsTpoxUwEzARBglghkgBhvhC
AQEEBAMCB4AwDQYJKoZIhvcNAQELBQADggEBAKlCnxW9XaO1mJEFfA8m1Yho+Hvd
VoLk9qCuBOKlW7s3fqo5S/cIpMSdDFeTqu52vQzO3wN8mvwKFAgRVrS5wGSH/Pe1
gWhgtx8lGz/TaQDV/wJSCyttSQrRUa6/GmVN52JNcLJYGGQLtqTaVXHct0uJ53ZK
PbOd06PbA1xyqBifvR7G1hTmqh/15R65UX+Gi06VaiekTtFYEizXaTFL7txvc5WY
AKzEETP8GPseAQokG0AorpNRHC8XAi2XVJ77S3Mg0eNWY0UWqtM2H5sk7gArLUQv
JtEuPvTexacUYU6HtSEZ/o3yV/zr0Sq40dacU4kKaLwFGgecFZIgGtZ6wpU=
-----END CERTIFICATE-----
EOF;

$cert_ns_server = <<<EOF
-----BEGIN CERTIFICATE-----
MIICaDCCAVCgAwIBAgIBbTANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjAUMRIwEAYDVQQDDAlOUyBTZXJ2ZXIwWTATBgcqhkjO
PQIBBggqhkjOPQMBBwNCAAS789403wtqilDpK77TLb8U+nlkMqyZvMSpvnPluJQv
iJX0YfvWx2hWNnROR0thlawh6aN/JKbD65QyvmwjNsTpoxUwEzARBglghkgBhvhC
AQEEBAMCB0AwDQYJKoZIhvcNAQELBQADggEBABsEC0HwGb8XQebNYrg+U7NVojAv
XpaYgmGtAEveRhCqZ0CkykA0zq1YnM0bg6rhsVqBn975nOCDtioaL0j40T1te70m
+yBtTDC/YHZGtSKvmrdqkiWY9aEdEHYqYUlkNoUPZtgtjmWCd7UYsxW5UvA5z9ll
1iUSUKpNKSFkngAo4Hdw69VSDm1QwFQRtmLv8u+uV8/ydVMhRKripf6sX4OCF833
SohCZSuDslUracTd7RgsM3w7Hqmb1GJt2k+gAkqb3fhHRkkLrrRYJs1UJOL2pZFC
SQGCjJ3z4o2X/JyOxRu8TkWSyZ4ugNH1VUbtOUr3Zp9Aity19kHe9oSGoLI=
-----END CERTIFICATE-----
EOF;

$cert_ns_smime = <<<EOF
-----BEGIN CERTIFICATE-----
MIICZzCCAU+gAwIBAgIBbjANBgkqhkiG9w0BAQsFADCBjjELMAkGA1UEBhMCVVMx
EzARBgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xGDAW
BgNVBAoMD0V4YW1wbGUgUm9vdCBDQTEeMBwGA1UECwwVQ2VydGlmaWNhdGUgQXV0
aG9yaXR5MRgwFgYDVQQDDA9FeGFtcGxlIFJvb3QgQ0EwHhcNMjMwNjE1MTIzMDQ1
WhcNMzMwNjE1MTIzMDQ1WjATMREwDwYDVQQDDAhOUyBTTUlNRTBZMBMGByqGSM49
AgEGCCqGSM49AwEHA0IABLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8xKm+c+W4lC+I
lfRh+9bHaFY2dE5HS2GVrCHpo38kpsPrlDK+bCM2xOmjFTATMBEGCWCGSAGG+EIB
AQQEAwIHIDANBgkqhkiG9w0BAQsFAAOCAQEAk+nmEAIhgCGtt7bJdw3Gun8P7OoP
U5DyI3TyIj7HskE0PX9q+qgdymrl5vhm0wsXg+gkCzqg3AioLiJ3MemxZ9zVjebN
va1zL6PhpBg9GGU3biJJlJgI1T99+KtKjLFVEGnvrMcca+u/EkIqfwDcuCxoMIxt
i2K9hLN64MW7qCd9qzrN+ry8IinHwkz+5PDGbJIYepaDEOb6tKgyqZws/E93fvfY
t2uwIOBec8Tr6glO106jS2X/pYMql4biNA9xmpku+eVJKtbXingtlh53sILq03Nq
Qq6tDsQ2CDoYE7TjGRoU7422vQ+l9xVnS781DKEFBEbd9S+DKYzQUvi3qw==
-----END CERTIFICATE-----
EOF;

$cert_v1 = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBMzCB2gIBBzAKBggqhkjOPQQDAjAmMQswCQYDVQQGEwJSVTEXMBUGA1UEAwwO
djEgY2VydGlmaWNhdGUwHhcNMjYxMTI2MTIwOTU5WhcNMzYxMTIzMTIwOTU5WjAm
MQswCQYDVQQGEwJSVTEXMBUGA1UEAwwOdjEgY2VydGlmaWNhdGUwWTATBgcqhkjO
PQIBBggqhkjOPQMBBwNCAAS789403wtqilDpK77TLb8U+nlkMqyZvMSpvnPluJQv
iJX0YfvWx2hWNnROR0thlawh6aN/JKbD65QyvmwjNsTpMAoGCCqGSM49BAMCA0gA
MEUCIDh0squbbA1op92Nl+61d1jtVmEGW/VXdQy4LtTe7qTMAiEA/QK9xvPAB897
WDxWNfix9jSlmz0C8/BWGcQc0F/qvAQ=
-----END CERTIFICATE-----
EOF;

$cert_v3_no_ext = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBODCB36ADAgECAgEIMAoGCCqGSM49BAMCMCYxCzAJBgNVBAYTAlJVMRcwFQYD
VQQDDA52MyBjZXJ0aWZpY2F0ZTAeFw0yMzA2MTUxMjMwNDVaFw0zMzA2MTUxMjMw
NDVaMCYxCzAJBgNVBAYTAlJVMRcwFQYDVQQDDA52MyBjZXJ0aWZpY2F0ZTBZMBMG
ByqGSM49AgEGCCqGSM49AwEHA0IABLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8xKm+
c+W4lC+IlfRh+9bHaFY2dE5HS2GVrCHpo38kpsPrlDK+bCM2xOkwCgYIKoZIzj0E
AwIDSAAwRQIhALmB14WM67Mqr7mgVcRgYVHUAEVpvkABTrzcleNxJGCXAiBl4yEX
z4uLIFb3C881Z23sHkalvnsK5tOpvsziNa6dOQ==
-----END CERTIFICATE-----
EOF;

$cert_ec_p256 = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBYDCCAQegAwIBAgIBCTAKBggqhkjOPQQDAjA6MQswCQYDVQQGEwJOTDEPMA0G
A1UECgwGRUMgMjU2MRowGAYDVQQDDBFlYzI1Ni5leGFtcGxlLm9yZzAeFw0yMzA2
MTUxMjMwNDVaFw0zMzA2MTUxMjMwNDVaMDoxCzAJBgNVBAYTAk5MMQ8wDQYDVQQK
DAZFQyAyNTYxGjAYBgNVBAMMEWVjMjU2LmV4YW1wbGUub3JnMFkwEwYHKoZIzj0C
AQYIKoZIzj0DAQcDQgAEu/PeNN8LaopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV
9GH71sdoVjZ0TkdLYZWsIemjfySmw+uUMr5sIzbE6TAKBggqhkjOPQQDAgNHADBE
AiBTWtBlIpzxcfhN8jfMg7D2dUyAZBej1d/aHfBt/w4EBgIgf8pANGnAvQxscohp
IvfZkKZy1ehaVvU6fiY8myA6EYI=
-----END CERTIFICATE-----
EOF;

$cert_ec_p384 = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBnzCCASSgAwIBAgIBCjAKBggqhkjOPQQDAzA6MQswCQYDVQQGEwJOTDEPMA0G
A1UECgwGRUMgMzg0MRowGAYDVQQDDBFlYzM4NC5leGFtcGxlLm9yZzAeFw0yMzA2
MTUxMjMwNDVaFw0zMzA2MTUxMjMwNDVaMDoxCzAJBgNVBAYTAk5MMQ8wDQYDVQQK
DAZFQyAzODQxGjAYBgNVBAMMEWVjMzg0LmV4YW1wbGUub3JnMHYwEAYHKoZIzj0C
AQYFK4EEACIDYgAE47eEO/TSYMFr1WebKeKdDi+G4soGo1OiWJ8OTH5uEy331JDe
9kI4l6mMC3iR8Tihj3Z2GnRsNlhvyRhsCkHRzVEav2N+1+3IaQeZyP4vheqj0IO0
wVr+WMghndAYjrKXMAoGCCqGSM49BAMDA2kAMGYCMQC4qAyADQKb7lte+YZ/q+Qk
rV6JAx82QBJfnNJ00Rv76cFErkwP0nkBlO3yfNSthsICMQD86+Thum9Sgnm0GNvA
+KXmDRvdzDh/GdLpm7MHvqguQ/ptmQHv0bcqVYyspc9wWCw=
-----END CERTIFICATE-----
EOF;

$cert_rsa_sha224 = <<<EOF
-----BEGIN CERTIFICATE-----
MIICozCCAYugAwIBAgIBCzANBgkqhkiG9w0BAQ4FADAVMRMwEQYDVQQDDApyc2Eg
c2hhMjI0MB4XDTIzMDYxNTEyMzA0NVoXDTMzMDYxNTEyMzA0NVowFTETMBEGA1UE
AwwKcnNhIHNoYTIyNDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKXm
xFn6T1OFRq78yqWG10DLDp3h9OZJHjsk9Lg3wJwHy/Vhpbiwu4Bb3RB7a8yzvjn9
BQczeFbRm+SrXXwovHabTEtuCsjn5EKJzrLmcR2EE7p8t1zDsLTtxOT63cxQMnc9
i/Yg1j5H2Fti1Cw44WIzf1uTT8qh47PLQCgJzlgZnzZb2TBGSRg4RMPJ6jHp7sZr
M6z/4RpEidHYS2oxgepdWhNz/tvaX2ogi1tfnTXAjjkyfumlfGsCNqYmJd6h48ER
/b5HHSQO/mlybQjVk0v5dxjoU1+q9Vf4U+xVCAq6afEnQKuR3rVxdnwMtZqbqy4N
FRLGzM+N1iMwMaeupHUCAwEAATANBgkqhkiG9w0BAQ4FAAOCAQEAF2v9EHeMpmrE
vDn6pUoRzaOAeRzoskikWvI4tjXjoNJdFeUDErTkIf8ROwXbM7HfhIhQ7fC48tXu
nC33vCQuoE5HQ2ZvGjiLcCNF46yTobry2Crl3tiRwYvbagyQm95O32Q+YenM8CuB
pOJaM7fCIbEIqTDM/3FBaPYyEPwnfAbo5roY4t4S0IFHKlWYKFoiuwWunpz+zRvS
Zh2G1wtDdoVUhTuYQPWnGqLnaxh2qiU16ZST7j42H/3ISp8SrOGkh/UXZPkwJJvJ
4vOsd3XH2jFysS/W98SUTQy3KuWe22WaLZPcKksMeC6mE9o5n7QpeUeNIXPkMmri
6P/JsWAQ0A==
-----END CERTIFICATE-----
EOF;

$cert_rsa_sha512 = <<<EOF
-----BEGIN CERTIFICATE-----
MIICozCCAYugAwIBAgIBDDANBgkqhkiG9w0BAQ0FADAVMRMwEQYDVQQDDApyc2Eg
c2hhNTEyMB4XDTIzMDYxNTEyMzA0NVoXDTMzMDYxNTEyMzA0NVowFTETMBEGA1UE
AwwKcnNhIHNoYTUxMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKXm
xFn6T1OFRq78yqWG10DLDp3h9OZJHjsk9Lg3wJwHy/Vhpbiwu4Bb3RB7a8yzvjn9
BQczeFbRm+SrXXwovHabTEtuCsjn5EKJzrLmcR2EE7p8t1zDsLTtxOT63cxQMnc9
i/Yg1j5H2Fti1Cw44WIzf1uTT8qh47PLQCgJzlgZnzZb2TBGSRg4RMPJ6jHp7sZr
M6z/4RpEidHYS2oxgepdWhNz/tvaX2ogi1tfnTXAjjkyfumlfGsCNqYmJd6h48ER
/b5HHSQO/mlybQjVk0v5dxjoU1+q9Vf4U+xVCAq6afEnQKuR3rVxdnwMtZqbqy4N
FRLGzM+N1iMwMaeupHUCAwEAATANBgkqhkiG9w0BAQ0FAAOCAQEAXy+2VbGemoNf
U71gr0wTGDWHbYpo1IbaupiFa8PbEmdGE3HEiCB1kKTog3p1xrA1auWzMjit9jS8
Jo+skrh7GO/OT9dkOPdIpB5fBkG1x9NLNnmTCPWmEmG5QdVdye6D5Fxwk2I68ODi
+X6o8mDjqHAmjiPKB0Yq52P1lN3iOiZ4PybTkOyVTnmEsIIyPClCjng8JjQ+a8sE
q6tmz71RgjQkkrjQZs87BOwyfqMnk+VCbVkm7pF8NQr7CuvD3MDSAKonW329UMjE
MJsScsyfVGv8YDw4gqg7Gza65Ajs6mSPLdr6sQjVgeykBlN5reYbn9mkkKfgS9bA
apqematukQ==
-----END CERTIFICATE-----
EOF;

$cert_many_fields = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDpzCCA02gAwIBAgIBFDAKBggqhkjOPQQDAjCCAVsxCzAJBgNVBAYTAkdCMRcw
FQYDVQQIDA5HcmVhdGVyIExvbmRvbjEPMA0GA1UEBwwGTG9uZG9uMRkwFwYDVQQJ
DBAxIEV4YW1wbGUgU3RyZWV0MQ8wDQYDVQQRDAZFMSA2QU4xFDASBgNVBAoMC0V4
YW1wbGUgTHRkMRQwEgYDVQQLDAtFbmdpbmVlcmluZzEUMBIGA1UEAwwLTWFueSBG
aWVsZHMxCzAJBgNVBAwMAkRyMQ0wCwYDVQQqDARKb2huMQ4wDAYDVQQEDAVTbWl0
aDERMA8GA1UEBRMIU04tMTIzNDUxFzAVBgoJkiaJk/IsZAEZFgdleGFtcGxlMR8w
HQYJKoZIhvcNAQkBFhBqb2huQGV4YW1wbGUuY29tMRYwFAYKCZImiZPyLGQBAQwG
anNtaXRoMRIwEAYDVQQuEwlxdWFsaWZpZXIxDzANBgNVBEEMBmpvaG5ueTAeFw0y
MzA2MTUxMjMwNDVaFw0zMzA2MTUxMjMwNDVaMIIBWzELMAkGA1UEBhMCR0IxFzAV
BgNVBAgMDkdyZWF0ZXIgTG9uZG9uMQ8wDQYDVQQHDAZMb25kb24xGTAXBgNVBAkM
EDEgRXhhbXBsZSBTdHJlZXQxDzANBgNVBBEMBkUxIDZBTjEUMBIGA1UECgwLRXhh
bXBsZSBMdGQxFDASBgNVBAsMC0VuZ2luZWVyaW5nMRQwEgYDVQQDDAtNYW55IEZp
ZWxkczELMAkGA1UEDAwCRHIxDTALBgNVBCoMBEpvaG4xDjAMBgNVBAQMBVNtaXRo
MREwDwYDVQQFEwhTTi0xMjM0NTEXMBUGCgmSJomT8ixkARkWB2V4YW1wbGUxHzAd
BgkqhkiG9w0BCQEWEGpvaG5AZXhhbXBsZS5jb20xFjAUBgoJkiaJk/IsZAEBDAZq
c21pdGgxEjAQBgNVBC4TCXF1YWxpZmllcjEPMA0GA1UEQQwGam9obm55MFkwEwYH
KoZIzj0CAQYIKoZIzj0DAQcDQgAEu/PeNN8LaopQ6Su+0y2/FPp5ZDKsmbzEqb5z
5biUL4iV9GH71sdoVjZ0TkdLYZWsIemjfySmw+uUMr5sIzbE6TAKBggqhkjOPQQD
AgNIADBFAiA9nJgnOKNyKvDGdzAaaFeVZVFFuuGcYWquTiPejj/cOwIhAPzHZgEJ
rIjHxF7DJ8/Ykl51W0JUuAa9walmewDBfK+3
-----END CERTIFICATE-----
EOF;

$cert_reordered = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBaTCCAQ+gAwIBAgIBFTAKBggqhkjOPQQDAjA+MRIwEAYDVQQDDAlyZW9yZGVy
ZWQxDTALBgNVBAsMBFVuaXQxDDAKBgNVBAoMA09yZzELMAkGA1UEBhMCRlIwHhcN
MjMwNjE1MTIzMDQ1WhcNMzMwNjE1MTIzMDQ1WjA+MRIwEAYDVQQDDAlyZW9yZGVy
ZWQxDTALBgNVBAsMBFVuaXQxDDAKBgNVBAoMA09yZzELMAkGA1UEBhMCRlIwWTAT
BgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtqilDpK77TLb8U+nlkMqyZvMSp
vnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD65QyvmwjNsTpMAoGCCqGSM49
BAMCA0gAMEUCIQD+d1oOTL2CWcOQJpy+fmiba6txefuf8nSwrqL07QavmgIgN5Cz
J+WwtWpbSI6SZedBeftbyN32gkIeL2WYUCqIewo=
-----END CERTIFICATE-----
EOF;

$cert_only_cn = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBIjCByaADAgECAgEWMAoGCCqGSM49BAMCMBsxGTAXBgNVBAMMEG9ubHkgY29t
bW9uIG5hbWUwHhcNMjMwNjE1MTIzMDQ1WhcNMzMwNjE1MTIzMDQ1WjAbMRkwFwYD
VQQDDBBvbmx5IGNvbW1vbiBuYW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE
u/PeNN8LaopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV9GH71sdoVjZ0TkdLYZWs
IemjfySmw+uUMr5sIzbE6TAKBggqhkjOPQQDAgNIADBFAiBovAPOckuI0Jm1wZDS
V0owPQon+yReEBBTXCf3ng2dnwIhAP/i+JxvBZSLh54Q1K0Ai6SlqmLOm6qkey1Q
VmWieTqT
-----END CERTIFICATE-----
EOF;

$cert_only_org = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBJTCBy6ADAgECAgEXMAoGCCqGSM49BAMCMBwxGjAYBgNVBAoMEW9ubHkgb3Jn
YW5pemF0aW9uMB4XDTIzMDYxNTEyMzA0NVoXDTMzMDYxNTEyMzA0NVowHDEaMBgG
A1UECgwRb25seSBvcmdhbml6YXRpb24wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC
AAS789403wtqilDpK77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0th
lawh6aN/JKbD65QyvmwjNsTpMAoGCCqGSM49BAMCA0kAMEYCIQCIL8jVBRrmFGP4
PswZzeV+w/1pNrwDiggSTrve4RQzVQIhALoUM0AOMFJXqvsQ2sXZ+ltI4Y5Co5H9
VPd5E7C1BQdb
-----END CERTIFICATE-----
EOF;

$cert_utf8 = <<<EOF
-----BEGIN CERTIFICATE-----
MIIByjCCAXGgAwIBAgIBGDAKBggqhkjOPQQDAjBvMQswCQYDVQQGEwJSVTEVMBMG
A1UEBwwM0JzQvtGB0LrQstCwMSowKAYDVQQKDCHQmtC+0LzQv9Cw0L3QuNGPIMKr
0J/RgNC40LzQtdGAwrsxHTAbBgNVBAMMFNCf0YDQuNCy0LXRgiwg0LzQuNGAMB4X
DTIzMDYxNTEyMzA0NVoXDTMzMDYxNTEyMzA0NVowbzELMAkGA1UEBhMCUlUxFTAT
BgNVBAcMDNCc0L7RgdC60LLQsDEqMCgGA1UECgwh0JrQvtC80L/QsNC90LjRjyDC
q9Cf0YDQuNC80LXRgMK7MR0wGwYDVQQDDBTQn9GA0LjQstC10YIsINC80LjRgDBZ
MBMGByqGSM49AgEGCCqGSM49AwEHA0IABLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8
xKm+c+W4lC+IlfRh+9bHaFY2dE5HS2GVrCHpo38kpsPrlDK+bCM2xOkwCgYIKoZI
zj0EAwIDRwAwRAIgQ8neUgn8MnBoXN6eNJ+n9C8r4Em1ODJYOSM7EtGWYZQCIDH9
ZneleO8suzkSqiKMZ2jmmmHU4i8RHAlqTFvDMy7E
-----END CERTIFICATE-----
EOF;

$cert_utf8_latin = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBqjCCAVGgAwIBAgIBGTAKBggqhkjOPQQDAjBfMQswCQYDVQQGEwJGUjESMBAG
A1UEBwwJQmVzYW7Dp29uMR0wGwYDVQQKDBRTb2Npw6l0w6kgR8OpbsOpcmFsZTEd
MBsGA1UEAwwUY2Fmw6kg5pel5pys6KqeIPCfmIAwHhcNMjMwNjE1MTIzMDQ1WhcN
MzMwNjE1MTIzMDQ1WjBfMQswCQYDVQQGEwJGUjESMBAGA1UEBwwJQmVzYW7Dp29u
MR0wGwYDVQQKDBRTb2Npw6l0w6kgR8OpbsOpcmFsZTEdMBsGA1UEAwwUY2Fmw6kg
5pel5pys6KqeIPCfmIAwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtq
ilDpK77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD
65QyvmwjNsTpMAoGCCqGSM49BAMCA0cAMEQCIGuDYMDoOVzyzLn/DgbklT6qljVv
05p1iii+Y3oYV/vKAiBCAxdvgPHaEzcPMTRS2i2Dz9HL8i+VVTVHpbINx8wZDA==
-----END CERTIFICATE-----
EOF;

$cert_special_chars = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBVTCB+6ADAgECAgEaMAoGCCqGSM49BAMCMDQxIjAgBgNVBAoMGUE9QiwgRDsg
IkYiICdHJyA8SD4gI0kgXEoxDjAMBgNVBAMMBWE9Yj1jMB4XDTIzMDYxNTEyMzA0
NVoXDTMzMDYxNTEyMzA0NVowNDEiMCAGA1UECgwZQT1CLCBEOyAiRiIgJ0cnIDxI
PiAjSSBcSjEOMAwGA1UEAwwFYT1iPWMwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC
AAS789403wtqilDpK77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0th
lawh6aN/JKbD65QyvmwjNsTpMAoGCCqGSM49BAMCA0kAMEYCIQDk3y8Tob1F20w2
NMaSwjtlYpLHV00W3by52VBVzBf5wAIhAKj3xJlWuy9x/8PWvBHA29H4ptsi7+bE
E+CvHF+9Lvvq
-----END CERTIFICATE-----
EOF;

$cert_spaces = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBTDCB86ADAgECAgEbMAoGCCqGSM49BAMCMDAxFDASBgNVBAoMC3R3byAgc3Bh
Y2VzMRgwFgYDVQQDDA90cmFpbGluZyBzcGFjZSAwHhcNMjMwNjE1MTIzMDQ1WhcN
MzMwNjE1MTIzMDQ1WjAwMRQwEgYDVQQKDAt0d28gIHNwYWNlczEYMBYGA1UEAwwP
dHJhaWxpbmcgc3BhY2UgMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEu/PeNN8L
aopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV9GH71sdoVjZ0TkdLYZWsIemjfySm
w+uUMr5sIzbE6TAKBggqhkjOPQQDAgNIADBFAiAsigFWQKqqm9M1KeyRcyod83Vg
ULs9rqj0Qmq+JWbWWwIhAJRC1GONaPLa3Bz9h1v2/2pWhfoD2p79jZ/f1yXeIvGR
-----END CERTIFICATE-----
EOF;

$cert_long_cn = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBhDCCASmgAwIBAgIBHDAKBggqhkjOPQQDAjBLMUkwRwYDVQQDDEB4eHh4eHh4
eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4
eHh4eHh4eHh4MB4XDTIzMDYxNTEyMzA0NVoXDTMzMDYxNTEyMzA0NVowSzFJMEcG
A1UEAwxAeHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4eHh4
eHh4eHh4eHh4eHh4eHh4eHh4eHh4eDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IA
BLvz3jTfC2qKUOkrvtMtvxT6eWQyrJm8xKm+c+W4lC+IlfRh+9bHaFY2dE5HS2GV
rCHpo38kpsPrlDK+bCM2xOkwCgYIKoZIzj0EAwIDSQAwRgIhAJiOw6+UuByKF0bd
id+6Tax5rWelhOVf0mOufd+DyNbXAiEAg4DWczxgXEnSfIYZLDqiWkU9CwvSSJVP
oqpF3otU0Do=
-----END CERTIFICATE-----
EOF;

$cert_empty_subject = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBETCBuKADAgECAgEeMAoGCCqGSM49BAMCMAAwHhcNMjMwNjE1MTIzMDQ1WhcN
MzMwNjE1MTIzMDQ1WjAAMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEu/PeNN8L
aopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV9GH71sdoVjZ0TkdLYZWsIemjfySm
w+uUMr5sIzbE6aMjMCEwHwYDVR0RAQH/BBUwE4IRZW1wdHkuZXhhbXBsZS5jb20w
CgYIKoZIzj0EAwIDSAAwRQIgeeRi5SPJbImoJfkG8mgYuWvaxrGl6rUJPwAep1aZ
ugoCIQDDmE+E/FPDHBuggzFNF7RFWnWUxPDE9SKx130AZFb9Kg==
-----END CERTIFICATE-----
EOF;

$cert_issuer_differs = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBWDCB4KADAgECAgEfMAoGCCqGSM49BAMCMDYxCzAJBgNVBAYTAkpQMRMwEQYD
VQQKDApJc3N1ZXIgT3JnMRIwEAYDVQQDDAlJc3N1ZXIgQ04wHhcNMjMwNjE1MTIz
MDQ1WhcNMzMwNjE1MTIzMDQ1WjAXMRUwEwYDVQQDDAxzdWJqZWN0IG9ubHkwWTAT
BgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtqilDpK77TLb8U+nlkMqyZvMSp
vnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD65QyvmwjNsTpMAoGCCqGSM49
BAMCA2cAMGQCMC5ssXrlavBZ2/sD/m5Bp913SJbwxPPLtwEOPiRRQeW6P0Wd4i4v
1gWXPkdMtNl8AQIwVfkWL4TCKmUCfxh14EH6YmXsZn0u6AzXnaC+Kv1byaTVrtwf
fNweJ464t+ShVas7
-----END CERTIFICATE-----
EOF;

$cert_date_epoch = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBGDCBv6ADAgECAgEoMAoGCCqGSM49BAMCMBYxFDASBgNVBAMMC2RhdGVzIGVw
b2NoMB4XDTcwMDEwMTAwMDAwMFoXDTcwMDEwMTAwMDAwMVowFjEUMBIGA1UEAwwL
ZGF0ZXMgZXBvY2gwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS789403wtqilDp
K77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/JKbD65Qy
vmwjNsTpMAoGCCqGSM49BAMCA0gAMEUCIQC4FzVcnSRwzLm1WAG1jAQvpjSvUtiY
3M1iGaKh2+4hOQIgCcbo5e2nJToTiLnVvl5Cbh4Cr/DQAO/zJQxRP02sgbg=
-----END CERTIFICATE-----
EOF;

$cert_date_y2k = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBFTCBu6ADAgECAgEqMAoGCCqGSM49BAMCMBQxEjAQBgNVBAMMCWRhdGVzIHky
azAeFw05OTEyMzEyMzU5NTlaFw0wMDAxMDEwMDAwMDBaMBQxEjAQBgNVBAMMCWRh
dGVzIHkyazBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABLvz3jTfC2qKUOkrvtMt
vxT6eWQyrJm8xKm+c+W4lC+IlfRh+9bHaFY2dE5HS2GVrCHpo38kpsPrlDK+bCM2
xOkwCgYIKoZIzj0EAwIDSQAwRgIhAKWyuaP311gsAbSalxUFaj9wTgIU/DI+k+fS
htacBSrzAiEAoajLp2lxqrJEX3lZuAIrSTyao+8QjfrWNaMMMbHfJx0=
-----END CERTIFICATE-----
EOF;

$cert_date_leap_day = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBHTCBxaADAgECAgEvMAoGCCqGSM49BAMCMBkxFzAVBgNVBAMMDmRhdGVzIGxl
YXBfZGF5MB4XDTI0MDIyOTEyMDAwMFoXDTI4MDIyOTEyMDAwMFowGTEXMBUGA1UE
AwwOZGF0ZXMgbGVhcF9kYXkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAS78940
3wtqilDpK77TLb8U+nlkMqyZvMSpvnPluJQviJX0YfvWx2hWNnROR0thlawh6aN/
JKbD65QyvmwjNsTpMAoGCCqGSM49BAMCA0cAMEQCIH1L6b+cO9G8IXRwITirFnhN
EBfHw8lPjel/aboDRKtkAiBYJxMsdOuaRhdgOY/tLIOR9gF3HwvhnBnu7SB4Ofs2
FA==
-----END CERTIFICATE-----
EOF;

$cert_date_one_second = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBIjCByaADAgECAgEwMAoGCCqGSM49BAMCMBsxGTAXBgNVBAMMEGRhdGVzIG9u
ZV9zZWNvbmQwHhcNMjUwNTA1MDUwNTA1WhcNMjUwNTA1MDUwNTA2WjAbMRkwFwYD
VQQDDBBkYXRlcyBvbmVfc2Vjb25kMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE
u/PeNN8LaopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV9GH71sdoVjZ0TkdLYZWs
IemjfySmw+uUMr5sIzbE6TAKBggqhkjOPQQDAgNIADBFAiEAszBY5qzL8tGRIJy3
8F+qnih4TkdTrDzIPcuptuV9DnYCIE0Q1Q+xUOy8RQYemLU8NfpK2xx9Cpxamzu8
U0SINaAN
-----END CERTIFICATE-----
EOF;

$cert_date_expired = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBHTCBw6ADAgECAgExMAoGCCqGSM49BAMCMBgxFjAUBgNVBAMMDWRhdGVzIGV4
cGlyZWQwHhcNMDEwMTAxMDAwMDAwWhcNMDIwMTAxMDAwMDAwWjAYMRYwFAYDVQQD
DA1kYXRlcyBleHBpcmVkMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEu/PeNN8L
aopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV9GH71sdoVjZ0TkdLYZWsIemjfySm
w+uUMr5sIzbE6TAKBggqhkjOPQQDAgNJADBGAiEAgbnfzhCukSJ1QQ4YvkWpyCCL
sYDfL+W5FyVwtJpWN/cCIQDu+Pdooi3KGWb9canSTIJDuBJ7uPDbFmfm9omngORE
mA==
-----END CERTIFICATE-----
EOF;

$cert_cli_rsa = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDlTCCAn2gAwIBAgIUfdRHig+A8EA3anREhi3NDFqXQDQwDQYJKoZIhvcNAQEL
BQAwWjELMAkGA1UEBhMCVVMxETAPBgNVBAgMCE5ldyBZb3JrMQwwCgYDVQQHDANO
WUMxEDAOBgNVBAoMB0NMSSBPcmcxGDAWBgNVBAMMD2NsaS5leGFtcGxlLmNvbTAe
Fw0yNjExMjYxMjA5NTlaFw0zNjExMjMxMjA5NTlaMFoxCzAJBgNVBAYTAlVTMREw
DwYDVQQIDAhOZXcgWW9yazEMMAoGA1UEBwwDTllDMRAwDgYDVQQKDAdDTEkgT3Jn
MRgwFgYDVQQDDA9jbGkuZXhhbXBsZS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IB
DwAwggEKAoIBAQC5IMSbc+L/ej0qqSwmdleVJXkbMK55xyxWmPFybWMO076HgzRX
zURX5LNyOVPn0dcglx3t4360POcRbZmBy+G3+Qe7EN53hkA+vsIDPMsfgF1NVda5
BL0ld/uEfL/sUghoaj7UHa7DuVmc3Loqjfevb9D+kbMEoaM+ohnpHityzHPh+bgL
HnWgG1H0X/YMqLswh6xysBafQ+b9ZPeKs4Anm/ZkXBOKayYW/CxZJrTMbUjlB1T2
Y3arZPXRzGBjTldkT8o3KzBUNmt/kh7+XKGS9hCzfNhK9Zwi5BURR81U4dRBD4V+
w77kX5mogk+5NPJ7JGH5dS+F8U0B/FsRbcp1AgMBAAGjUzBRMB0GA1UdDgQWBBT7
/YF9RRdWroSDAqJ97uYWPHTj9TAfBgNVHSMEGDAWgBT7/YF9RRdWroSDAqJ97uYW
PHTj9TAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAkVdi9cO9U
vy2yRwNmKcPoHEDKRhDf4+t92sxPX3TM1VRYmgrTlANL0wGOmb8WmdhSYHfDKwyU
WSMGkduh/YcaLJodPEpQHX5RZ5DlNL6DFS1ZX5aiRsyQfQmWaxr4DFs430JYWh1b
qzKTzf/y46h32iRlhrl0sV6mZyy6r5H7sHkahD2BOjQaKqpxgoUphy76K8a7N25A
o4eo509hdhk0Zea3eYlm/ZGzId3elBLIKCPsASwLyX9iG+ZPr8iFOncnIGqvViWo
pfbZbHRETNg5Sr3FeZTM+OxM2SZJRbxU/cAkJ0Eba74qMjBNHUEBtZjZYKAuTqDd
4sYOFOam/2xb
-----END CERTIFICATE-----
EOF;

$cert_multivalued_rdn = <<<EOF
-----BEGIN CERTIFICATE-----
MIIBTDCB86ADAgECAgEdMAoGCCqGSM49BAMCMDAxCzAJBgNVBAYTAlNFMSEwDAYD
VQQKDAVNdWx0aTARBgNVBAMMCnZhbHVlZCByZG4wHhcNMjMwNjE1MTIzMDQ1WhcN
MzMwNjE1MTIzMDQ1WjAwMQswCQYDVQQGEwJTRTEhMAwGA1UECgwFTXVsdGkwEQYD
VQQDDAp2YWx1ZWQgcmRuMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEu/PeNN8L
aopQ6Su+0y2/FPp5ZDKsmbzEqb5z5biUL4iV9GH71sdoVjZ0TkdLYZWsIemjfySm
w+uUMr5sIzbE6TAKBggqhkjOPQQDAgNIADBFAiBBbZqeJYXrz6UhRWjdX0LXU5V6
+v34Mlifc6Z+XiDgXQIhAK9TwVdBEA8R2S5zhJgLNJq1jV1a8ZGlSvUePixyly4o
-----END CERTIFICATE-----
EOF;

$all_certs = [
    'ca_root' => $cert_ca_root,
    'ca_pathlen' => $cert_ca_pathlen,
    'ca_same_subject' => $cert_ca_same_subject,
    'server' => $cert_server,
    'client' => $cert_client,
    'smime' => $cert_smime,
    'code_signing' => $cert_code_signing,
    'all_eku' => $cert_all_eku,
    'ocsp_signer' => $cert_ocsp_signer,
    'key_agreement_only' => $cert_key_agreement_only,
    'crl_signer' => $cert_crl_signer,
    'ns_client' => $cert_ns_client,
    'ns_server' => $cert_ns_server,
    'ns_smime' => $cert_ns_smime,
    'v1' => $cert_v1,
    'v3_no_ext' => $cert_v3_no_ext,
    'ec_p256' => $cert_ec_p256,
    'ec_p384' => $cert_ec_p384,
    'rsa_sha224' => $cert_rsa_sha224,
    'rsa_sha512' => $cert_rsa_sha512,
    'many_fields' => $cert_many_fields,
    'reordered' => $cert_reordered,
    'only_cn' => $cert_only_cn,
    'only_org' => $cert_only_org,
    'utf8' => $cert_utf8,
    'utf8_latin' => $cert_utf8_latin,
    'special_chars' => $cert_special_chars,
    'spaces' => $cert_spaces,
    'long_cn' => $cert_long_cn,
    'empty_subject' => $cert_empty_subject,
    'issuer_differs' => $cert_issuer_differs,
    'date_epoch' => $cert_date_epoch,
    'date_y2k' => $cert_date_y2k,
    'date_leap_day' => $cert_date_leap_day,
    'date_one_second' => $cert_date_one_second,
    'date_expired' => $cert_date_expired,
    'cli_rsa' => $cert_cli_rsa,
];

$private_key_pem = <<<EOF
-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCyvzvXoOe04/Jn
dj64xVNgLUcyMoigBtdxKQaCezBGLQ98Y9CEQoie8Gy5PQTI2NYGk5jWAwER5stv
JuHqscrDsrgZfaEKmcBaAzRImZ9ImSKW0qGHVCnJTnqPNb1gufxY0xy8QSSS1tno
2/AHcKR5sL0mEewSOe2qmPdxqaSDQXZJCS/0Tm6KMzmGZagaLKZWY3L51L8KR+PX
STuFVtOVQaBIes0YAMbyzsHtzYiJBIFDnbLMhkF6lcfVzb2/6jIWeBgyHSYYqDd6
9Hr0DFHe1GzcocpXDRoQrikzHFxnjBzkfcTdhqn0fD24JEIVCh7jRNod5ThpGoW6
8B1ljsWnAgMBAAECggEAVaaL+s2n6cZmc8w1zt0DEC1yFawgSqoslxVFAMYdwiq6
Zmz/0Dh/dcWYG4URI1q/58PW8wnae+T/K0JS896A3hhn3hMUVVuKT7eEWJuAN03l
xKambw3VvhgEY1Ypak2EjV2PwednKpOYNFwbXr1LmuIlKLZiy/MFHv+QZl8XLSsl
n8ROt96K+NJ7pVmLJCivrU5d2rCONooBMveUSLCO0qEom0EfiDXiX9rPG8QUBDAt
XlccqwNKeT/7wEb/3h1V/1qDdKLY06pQqx3TfZ0hqT0tYP17Ga1SBKB0iM2ipQbe
Cd+VcGlGz3fXJmUmMYAG5Hey1uo56D9D8dG6p0tNDQKBgQDrO1mcpkFMjkpd6QJT
fG9PxBZDUj7TVBoO3lofB6wSSoDtkR7rtO2zKwwIp24QG3AZNT0nFSbKMCNL1vFS
z8QHyq/E0jOhAoU9O1J9L+NerDy1D9mpaUAJnY3QRetSE2gOHGgyFIijGKu/cNl/
n/pe7bMxhe+8PyRxccsbzS3jpQKBgQDChzpe6jZPQMDU/yxbnfcWhYkCC0GbGJ3A
lac0dJhdN5Ar8djquOxoYzV1cRy0Hlt/R+wFiLg3aWeUw12wtr3IViO170c4AWrS
dJFLkat5CqY9Xk68sfqBeRPXSKnNynl9XnuZyaadmTx082K0i3QB7+oz33iwKlv6
JlzhRcRSWwKBgQCsbDaFUoAuw2XTbEZeN6nHyC2hbxnKAa2QFH/umssQJThX6rXF
hDtvxy/s7/0HJXWHpLSMfz6NmbdlkcoWL2C7bWBlXsalUJmlhmQyivWYoZtmBJZB
TXq5EZmvfnC9Gqh9H2lovBBjrGPH0co7eGInTwi4Vjqhu47U8No2KLFoWQKBgGsv
deM8rGYo6Ne0Lq3gsXgwZhjBOQoLm0i1bSdDI30hk4gTP5G+CU1PdPJcPBOnMoVE
jPFTryWhJ5jZuTMYziOoCw9BY/2hEkoaPsDj6BbO3mCsKThh3QUgQOV7vUyORzjk
KbjIWs011REf1oBi4q36kxK4DNkMzhdcsgrSlWVHAoGBAMohdRufUOeA5pTMtf5v
hc8F5yhWqjaysJ1mkQYqhVmEt+6r5eHLfAT0JE4+Eyj6iJzMxVQBqq3Pxj4yyyuR
jdj7EtoIyMNIz9bmBTjQ7W/a8LTfNcl7tt+EEaFTYwaneVWQ83crXAnQX+E/TBbE
mAZuCbeBX0PubAhEMT6RXk+O
-----END PRIVATE KEY-----
EOF;

$public_key_pem = <<<EOF
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAsr8716DntOPyZ3Y+uMVT
YC1HMjKIoAbXcSkGgnswRi0PfGPQhEKInvBsuT0EyNjWBpOY1gMBEebLbybh6rHK
w7K4GX2hCpnAWgM0SJmfSJkiltKhh1QpyU56jzW9YLn8WNMcvEEkktbZ6NvwB3Ck
ebC9JhHsEjntqpj3camkg0F2SQkv9E5uijM5hmWoGiymVmNy+dS/Ckfj10k7hVbT
lUGgSHrNGADG8s7B7c2IiQSBQ52yzIZBepXH1c29v+oyFngYMh0mGKg3evR69AxR
3tRs3KHKVw0aEK4pMxxcZ4wc5H3E3Yap9Hw9uCRCFQoe40TaHeU4aRqFuvAdZY7F
pwIDAQAB
-----END PUBLIC KEY-----
EOF;

function dump_cert(string $pem, bool $shortnames = true) {
    $result = openssl_x509_parse($pem, $shortnames);
    if ($result === false) {
        var_dump(false);
        return;
    }

    var_dump($result['name']);
    var_dump($result['subject']);
    var_dump($result['hash']);
    var_dump($result['issuer']);
    var_dump($result['version']);
    var_dump($result['validFrom_time_t']);
    var_dump($result['validTo_time_t']);
    var_dump($result['purposes']);
}

function purpose_flags(string $pem, int $id) {
    $result = openssl_x509_parse($pem);
    if ($result === false) {
        return 'parse failed';
    }

    $purpose = $result['purposes'][$id];

    return ($purpose[0] ? 'T' : 'F') . ($purpose[1] ? 'T' : 'F');
}

function test_all_certs_shortnames() {
    global $all_certs;

    foreach ($all_certs as $pem) {
        dump_cert($pem, true);
    }
}

function test_all_certs_longnames() {
    global $all_certs;

    foreach ($all_certs as $pem) {
        dump_cert($pem, false);
    }
}

function test_default_shortnames() {
    global $all_certs;

    foreach ($all_certs as $pem) {
        $default = openssl_x509_parse($pem);
        $short = openssl_x509_parse($pem, true);
        $long = openssl_x509_parse($pem, false);
        var_dump($default !== false && $short !== false && $long !== false);
        var_dump($default === $short);
        var_dump($default['name'] === $long['name']);
        var_dump($default['hash'] === $long['hash']);
        var_dump($default['version'] === $long['version']);
        var_dump($default['validFrom_time_t'] === $long['validFrom_time_t']);
        var_dump($default['validTo_time_t'] === $long['validTo_time_t']);
    }
}

function test_result_structure() {
    global $all_certs;

    $keys = ['name', 'subject', 'hash', 'issuer', 'version', 'validFrom_time_t', 'validTo_time_t', 'purposes'];
    foreach ($all_certs as $pem) {
        $result = openssl_x509_parse($pem);
        var_dump($result !== false);
        foreach ($keys as $key) {
            var_dump(isset($result[$key]));
        }
        var_dump(is_string($result['name']));
        var_dump(is_array($result['subject']));
        var_dump(is_string($result['hash']));
        var_dump(strlen($result['hash']));
        var_dump(ctype_xdigit($result['hash']));
        var_dump($result['hash'] === strtolower($result['hash']));
        var_dump(is_array($result['issuer']));
        var_dump(is_int($result['version']));
        var_dump(is_int($result['validFrom_time_t']));
        var_dump(is_int($result['validTo_time_t']));
        var_dump(is_array($result['purposes']));
        var_dump(count($result['purposes']));
        foreach ($result['purposes'] as $id => $purpose) {
            var_dump(is_int($id));
            var_dump(count($purpose));
            var_dump(is_bool($purpose[0]));
            var_dump(is_bool($purpose[1]));
            var_dump(is_string($purpose[2]));
        }
    }
}

function test_subject_and_issuer() {
    global $cert_ca_root, $cert_server, $cert_issuer_differs, $cert_ca_same_subject, $cert_v1, $cert_ec_p256;

    foreach ([$cert_ca_root, $cert_v1, $cert_ec_p256, $cert_ca_same_subject] as $pem) {
        $result = openssl_x509_parse($pem);
        var_dump($result['subject'] === $result['issuer']);
    }

    $root = openssl_x509_parse($cert_ca_root);
    $server = openssl_x509_parse($cert_server);
    var_dump($server['subject'] === $server['issuer']);
    var_dump($server['issuer'] === $root['subject']);
    var_dump($server['issuer'] === $root['issuer']);

    $differs = openssl_x509_parse($cert_issuer_differs);
    var_dump($differs['subject'] === $differs['issuer']);
    var_dump(count($differs['subject']));
    var_dump(count($differs['issuer']));
    var_dump($differs['subject']['CN']);
    var_dump($differs['issuer']['CN']);
    var_dump($differs['issuer']['C']);
    var_dump($differs['issuer']['O']);

    $short = openssl_x509_parse($cert_ca_root, true);
    $long = openssl_x509_parse($cert_ca_root, false);
    var_dump($short['subject']['C']);
    var_dump($short['subject']['ST']);
    var_dump($short['subject']['L']);
    var_dump($short['subject']['O']);
    var_dump($short['subject']['OU']);
    var_dump($short['subject']['CN']);
    var_dump($long['subject']['countryName']);
    var_dump($long['subject']['stateOrProvinceName']);
    var_dump($long['subject']['localityName']);
    var_dump($long['subject']['organizationName']);
    var_dump($long['subject']['organizationalUnitName']);
    var_dump($long['subject']['commonName']);
    var_dump(isset($short['subject']['commonName']));
    var_dump(isset($long['subject']['CN']));
    var_dump(array_keys($short['subject']));
    var_dump(array_keys($long['subject']));
    var_dump(array_values($short['subject']) === array_values($long['subject']));
}

function test_subject_key_order() {
    global $cert_reordered, $cert_many_fields, $cert_multivalued_rdn;

    var_dump(array_keys(openssl_x509_parse($cert_reordered)['subject']));
    var_dump(array_values(openssl_x509_parse($cert_reordered)['subject']));
    var_dump(array_keys(openssl_x509_parse($cert_many_fields)['subject']));
    var_dump(array_keys(openssl_x509_parse($cert_many_fields, false)['subject']));
    var_dump(array_keys(openssl_x509_parse($cert_multivalued_rdn)['subject']));
    $multi = openssl_x509_parse($cert_multivalued_rdn);
    var_dump($multi['subject']);
    var_dump($multi['issuer']);
    var_dump($multi['hash']);
    var_dump($multi['version']);
}

function test_utf8_subject() {
    global $cert_utf8, $cert_utf8_latin;

    foreach ([$cert_utf8, $cert_utf8_latin] as $pem) {
        $result = openssl_x509_parse($pem);
        var_dump($result['subject']);
        var_dump($result['name']);
        var_dump(bin2hex($result['subject']['CN']));
        var_dump(mb_check_encoding($result['subject']['CN'], 'UTF-8'));
        var_dump(strlen($result['subject']['O']));
    }
}

function test_long_values() {
    global $cert_long_cn;

    $result = openssl_x509_parse($cert_long_cn);
    var_dump(strlen($result['subject']['CN']));
    var_dump($result['subject']['CN'] === str_repeat('x', 64));
    var_dump(strlen($result['name']));
}

function test_hash() {
    global $cert_ca_root, $cert_ca_same_subject, $cert_server, $cert_client, $cert_only_cn, $all_certs;

    $root = openssl_x509_parse($cert_ca_root);
    $same = openssl_x509_parse($cert_ca_same_subject);
    $server = openssl_x509_parse($cert_server);
    $client = openssl_x509_parse($cert_client);

    var_dump($root['hash']);
    var_dump($server['hash']);
    var_dump($client['hash']);
    var_dump($root['hash'] === $same['hash']);
    var_dump($root['hash'] === $server['hash']);
    var_dump($server['hash'] === $client['hash']);

    $hashes = [];
    foreach ($all_certs as $label => $pem) {
        $hashes[$label] = openssl_x509_parse($pem)['hash'];
    }

    var_dump($hashes);
    var_dump(count($hashes) === count(array_unique($hashes)) + 1);
}

function test_validity_order() {
    global $all_certs;

    foreach ($all_certs as $pem) {
        $result = openssl_x509_parse($pem);
        var_dump($result['validFrom_time_t'] < $result['validTo_time_t']);
        var_dump($result['validTo_time_t'] - $result['validFrom_time_t']);
    }
}

function test_validity_boundaries() {
    global $cert_date_epoch, $cert_date_leap_day, $cert_date_one_second;

    $epoch = openssl_x509_parse($cert_date_epoch);
    var_dump($epoch['validFrom_time_t']);
    var_dump($epoch['validTo_time_t']);

    $leap = openssl_x509_parse($cert_date_leap_day);
    var_dump(gmdate('Y-m-d H:i:s', $leap['validFrom_time_t']));
    var_dump(gmdate('Y-m-d H:i:s', $leap['validTo_time_t']));

    $one = openssl_x509_parse($cert_date_one_second);
    var_dump($one['validTo_time_t'] - $one['validFrom_time_t']);
    var_dump(gmdate('Y-m-d H:i:s', $one['validFrom_time_t']));
}

function test_validity_in_utc() {
    global $cert_server, $cert_ca_root, $cert_ca_pathlen;

    $server = openssl_x509_parse($cert_server);
    var_dump(gmdate('Y-m-d H:i:s', $server['validFrom_time_t']));
    var_dump(gmdate('Y-m-d H:i:s', $server['validTo_time_t']));
    $root = openssl_x509_parse($cert_ca_root);
    var_dump(gmdate('Y-m-d H:i:s', $root['validFrom_time_t']));
    var_dump(gmdate('Y-m-d H:i:s', $root['validTo_time_t']));
    $inter = openssl_x509_parse($cert_ca_pathlen);
    var_dump(gmdate('Y-m-d H:i:s', $inter['validFrom_time_t']));
    var_dump(gmdate('Y-m-d H:i:s', $inter['validTo_time_t']));
}

function test_purposes_flags() {
    global $cert_ca_root, $cert_ca_pathlen, $cert_server, $cert_client, $cert_smime, $cert_code_signing, $cert_all_eku,
           $cert_ocsp_signer, $cert_key_agreement_only, $cert_crl_signer, $cert_ns_client, $cert_ns_server, $cert_ns_smime,
           $cert_v1, $cert_v3_no_ext, $cert_ec_p256;

    $certs = [
        'ca_root' => $cert_ca_root, 'ca_pathlen' => $cert_ca_pathlen, 'server' => $cert_server, 'client' => $cert_client,
        'smime' => $cert_smime, 'code_signing' => $cert_code_signing, 'all_eku' => $cert_all_eku, 'ocsp_signer' => $cert_ocsp_signer,
        'key_agreement_only' => $cert_key_agreement_only, 'crl_signer' => $cert_crl_signer, 'ns_client' => $cert_ns_client,
        'ns_server' => $cert_ns_server, 'ns_smime' => $cert_ns_smime, 'v1' => $cert_v1, 'v3_no_ext' => $cert_v3_no_ext, 'ec_p256' => $cert_ec_p256,
    ];

    foreach ($certs as $label => $pem) {
        echo "$label:";
        for ($id = 1; $id <= 9; ++$id) {
            echo ' ', purpose_flags($pem, $id);
        }
        echo "\n";
    }
}

function test_pem_formatting() {
    global $cert_server;

    $expected = openssl_x509_parse($cert_server);
    var_dump($expected !== false);

    var_dump(openssl_x509_parse("some text before\n\n" . $cert_server) === $expected);
    var_dump(openssl_x509_parse($cert_server . "\ntrailing text") === $expected);
    var_dump(openssl_x509_parse("text\n" . $cert_server . "\ntext") === $expected);
    var_dump(openssl_x509_parse(str_replace("\n", "\r\n", $cert_server)) === $expected);
    var_dump(openssl_x509_parse(rtrim($cert_server)) === $expected);
    var_dump(openssl_x509_parse($cert_server . "\n\n\n\n") === $expected);
    var_dump(openssl_x509_parse("\n\n\n" . $cert_server) === $expected);
    var_dump(openssl_x509_parse($cert_server . "\n" . $cert_server) === $expected);
    var_dump(openssl_x509_parse("Certificate:\n    Data:\n        Version: 3\n" . $cert_server) === $expected);
    var_dump(openssl_x509_parse('  ' . $cert_server) === false);
    var_dump(openssl_x509_parse(str_replace("-----BEGIN CERTIFICATE-----\n", "-----BEGIN CERTIFICATE-----\n\n", $cert_server)) === $expected);
}

function test_several_certificates() {
    global $cert_ca_root, $cert_server, $cert_client;

    $root = openssl_x509_parse($cert_ca_root);
    $server = openssl_x509_parse($cert_server);
    $chain = $cert_server . "\n" . $cert_ca_root;
    $parsed = openssl_x509_parse($chain);
    var_dump($parsed === $server);
    var_dump($parsed['name']);

    $reversed = openssl_x509_parse($cert_ca_root . "\n" . $cert_server . "\n" . $cert_client);
    var_dump($reversed === $root);
    var_dump($reversed['name']);
}

function test_invalid_input() {
    global $cert_server, $cert_v1, $private_key_pem, $public_key_pem;

    $lines = explode("\n", rtrim($cert_server));
    $body = implode("\n", array_slice($lines, 1, count($lines) - 2));
    $broken_first_line = $lines;
    $broken_first_line[1] = 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA';
    $der = base64_decode(str_replace("\n", '', $body));

    $invalid = [
        '',
        ' ',
        "\n",
        'garbage',
        'not a certificate',
        "\0",
        "\0\0\0\0",
        str_repeat('A', 1000),
        '-----BEGIN CERTIFICATE-----',
        '-----BEGIN CERTIFICATE-----' . "\n" . '-----END CERTIFICATE-----',
        '-----BEGIN CERTIFICATE-----' . "\n" . "\n" . '-----END CERTIFICATE-----',
        "-----BEGIN CERTIFICATE-----\nAAAA\n-----END CERTIFICATE-----",
        "-----BEGIN CERTIFICATE-----\n" . $body,
        $body,
        $body . "\n-----END CERTIFICATE-----",
        "-----BEGIN CERTIFICATE-----\n" . implode("\n", array_slice($lines, 1, 2)) . "\n-----END CERTIFICATE-----",
        implode("\n", $broken_first_line),
        "-----BEGIN CERTIFICATE-----\n!!!!" . substr($body, 4) . "\n-----END CERTIFICATE-----",
        str_replace('CERTIFICATE', 'PUBLIC KEY', $cert_server),
        str_replace('CERTIFICATE', 'PRIVATE KEY', $cert_server),
        str_replace('CERTIFICATE', 'CERTIFICATE REQUEST', $cert_server),
        str_replace('BEGIN', 'END', $cert_server),
        substr($cert_server, 0, 100),
        substr($cert_server, 0, strlen($cert_server) >> 1),
        substr($cert_server, 0, strlen($cert_server) - 40),
        $private_key_pem,
        $public_key_pem,
        $der,
        substr($der, 0, 100),
        bin2hex($der),
        base64_encode($der),
        'file:///nonexistent/path/cert.pem',
        '/nonexistent/path/cert.pem',
    ];
    
    foreach ($invalid as $i => $input) {
        var_dump(openssl_x509_parse($input) === false);
        var_dump(openssl_x509_parse($input, false) === false);
    }
}

function test_not_a_certificate_after_valid() {
    global $cert_server, $private_key_pem;

    // text before is skipped, but the first PEM block that is not a certificate is not
    var_dump(openssl_x509_parse($private_key_pem . "\n" . $cert_server) !== false);
    var_dump(openssl_x509_parse($private_key_pem . "\n" . $cert_server) === openssl_x509_parse($cert_server));
}

function test_repeated_parsing() {
    global $cert_server, $cert_ec_p256, $cert_utf8;

    $expected = openssl_x509_parse($cert_server);
    $same = 0;
    for ($i = 0; $i < 300; ++$i) {
        if (openssl_x509_parse($cert_server) === $expected) {
            ++$same;
        }
    }

    var_dump($same);

    $same = 0;
    for ($i = 0; $i < 100; ++$i) {
        $a = openssl_x509_parse($cert_ec_p256, $i % 2 === 0);
        $b = openssl_x509_parse($cert_utf8, $i % 2 === 1);
        if ($a !== false && $b !== false && $a['hash'] !== $b['hash']) {
            ++$same;
        }
    }
    
    var_dump($same);

    $failed = 0;
    for ($i = 0; $i < 100; ++$i) {
        if (openssl_x509_parse('invalid ' . $i) === false) {
            ++$failed;
        }
    }

    var_dump($failed);
    var_dump(openssl_x509_parse($cert_server) === $expected);
}

function test_same_certificate_different_order() {
    global $all_certs;

    // parse results must not depend on what was parsed before
    $labels = array_keys($all_certs);
    $first = [];
    foreach ($labels as $label) {
        $first[$label] = openssl_x509_parse($all_certs[$label]);
    }

    $reversed = array_reverse($labels);
    $equal = 0;
    foreach ($reversed as $label) {
        if (openssl_x509_parse($all_certs[$label]) === $first[$label]) {
            ++$equal;
        }
    }
    
    var_dump($equal === count($labels));
}

test_all_certs_shortnames();
test_all_certs_longnames();
test_default_shortnames();
test_result_structure();
test_subject_and_issuer();
test_subject_key_order();
test_utf8_subject();
test_long_values();
test_hash();
test_validity_order();
test_validity_boundaries();
test_validity_in_utc();
test_purposes_flags();
test_pem_formatting();
test_several_certificates();
test_invalid_input();
test_not_a_certificate_after_valid();
test_repeated_parsing();
test_same_certificate_different_order();
