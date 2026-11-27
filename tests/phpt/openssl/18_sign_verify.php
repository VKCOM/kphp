@ok
<?php

$rsa2048_private = <<<EOF
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCILnpcP49RueO9
K+c8zXxcFg08RzTDUb08lByhg3NmcESJJeqeOE82RK7pXipBV/wIck5JKLmCwmR7
JLgdBEfvdCgoJGT6TTLI0yZgNkEoOs+RnE3zc5EaOyzPQAcZBXpnh90izrgBTyp6
za77iwnShL3/bBVbO5dNcXYxaIBFd3gkHJLHZ1rsREh1vBy/b207rZbtNL6jYzYB
FXvEm4iajF8uvZWItR9VQAssUNDDfhLIp+DPEDKJZqeqBH8nDsJZOl36Nag5vRPF
KVwPztthlNc+/Ee3hThf5raWb8fIVSs7EkQksuZynOzpoSwnHWmnvtL7bZmvaeV4
fJDg8QQJAgMBAAECggEAEsbjJOBiq4XHtVKzYeUstinC420nWkhlS6VZw+9dtGHb
IVSdYqQoT+GX2HedDet1cINfm04b3hVdnhkfBb6MXfjp0eHKxom/Fy+or/AV0IKO
Ej63WiUKJyVs9UVCdUcVweawLVd2yZTCf7I9OVPKI+vHostNWon/P7SwSPm8DgJ1
NTJLmjbt1as5FpbV+7h/726dsBHmqlQ35YFM1Kyk3XNumeNNvb+mJDcAx7v8+6Mx
bkntDJFSesxzYu6E+OwH7rJKLKYT2CWCfZASgZQlMpEtQgcz0Jl7Nmb11fu7QIwE
r74wmshQQm7PPY9qaBPkoUYsyqkbX0FSxfI/xtyr3QKBgQC+CC9knnUc3P3+MlHw
C6Y4t4jlzaQY6C9nUhRjZ266BbNLkh+rb94BllHVgjggJ9hHN0y4LWZHggjB6J7S
pDQ1co0pXgUr0P2pi5MpzAQKHZ5LEesOl+uKKVBG+p0Olmip66lNBPpPzHWKhguK
o/jGFNzBO0BZwF/Sc1Vjw/VV4wKBgQC3dLLdI++H4V5n5uQQDePm3YmU0i3NQ5sE
Uf224YKfUv2yTGEcFrRZTxa2W0UU6DWbLQKi9MrkmCxXysSEKBfvtIkWnD0p8+7L
CUSakTDBYWWY0wfcGj86WLYG1hKVCbc/VhIzCIo4a8N7nHZO6BzKVYG8W+tkDcRo
6zFOrmOCIwKBgBAKaNW5enFBRj9ayAfXM+S9uVSMkzl2/+lcyFPLu1EAgi3y27GA
8+B2a9n17/7m3h7E0DkLvAJ1O4ACvsVRDwcj8D55rT4RmhfhrD5u9jhUX7Q+gvTs
Zfl3m0sihHdiwtr1BYl2zhpGS/Wxcu/0/17ZC/lTow7waiY9jgup0t+5AoGBAIV/
TDET11gfrZ0aLVdpZDA5gcZj86Rsz78jEucjzvO1Zpt6bSgNdYuPu81TOT/vgiK7
bDG8xG/4NDxmdv5cVp691ww3AWMn+NqSkfOQN9Rg1ZeR6SRHjT8eEseCu97sRQa3
0WKFkl9gO84VmxnsWhb6hf/VO8OBKS3YB/dje1HVAoGAXbcd3Frvrn941UN/f9dL
uAd7Pc/RRJ2A948xj7hlQJHVHs3Lu8X4misLPOMVImCtJ2lo10/wOyHIxwzGNjdX
VYEXd5FALCMrfMMt0ScORM+RD5JB+vVZsggxav6oAl9VknmCP8CP+ovvCtSv5mAk
HXwDmuSC1cCdGCsm2PMBrpo=
-----END PRIVATE KEY-----
EOF;

$rsa2048_public = <<<EOF
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAiC56XD+PUbnjvSvnPM18
XBYNPEc0w1G9PJQcoYNzZnBEiSXqnjhPNkSu6V4qQVf8CHJOSSi5gsJkeyS4HQRH
73QoKCRk+k0yyNMmYDZBKDrPkZxN83ORGjssz0AHGQV6Z4fdIs64AU8qes2u+4sJ
0oS9/2wVWzuXTXF2MWiARXd4JBySx2da7ERIdbwcv29tO62W7TS+o2M2ARV7xJuI
moxfLr2ViLUfVUALLFDQw34SyKfgzxAyiWanqgR/Jw7CWTpd+jWoOb0TxSlcD87b
YZTXPvxHt4U4X+a2lm/HyFUrOxJEJLLmcpzs6aEsJx1pp77S+22Zr2nleHyQ4PEE
CQIDAQAB
-----END PUBLIC KEY-----
EOF;

$rsa2048_pkcs1_private = <<<EOF
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAiC56XD+PUbnjvSvnPM18XBYNPEc0w1G9PJQcoYNzZnBEiSXq
njhPNkSu6V4qQVf8CHJOSSi5gsJkeyS4HQRH73QoKCRk+k0yyNMmYDZBKDrPkZxN
83ORGjssz0AHGQV6Z4fdIs64AU8qes2u+4sJ0oS9/2wVWzuXTXF2MWiARXd4JByS
x2da7ERIdbwcv29tO62W7TS+o2M2ARV7xJuImoxfLr2ViLUfVUALLFDQw34SyKfg
zxAyiWanqgR/Jw7CWTpd+jWoOb0TxSlcD87bYZTXPvxHt4U4X+a2lm/HyFUrOxJE
JLLmcpzs6aEsJx1pp77S+22Zr2nleHyQ4PEECQIDAQABAoIBABLG4yTgYquFx7VS
s2HlLLYpwuNtJ1pIZUulWcPvXbRh2yFUnWKkKE/hl9h3nQ3rdXCDX5tOG94VXZ4Z
HwW+jF346dHhysaJvxcvqK/wFdCCjhI+t1olCiclbPVFQnVHFcHmsC1XdsmUwn+y
PTlTyiPrx6LLTVqJ/z+0sEj5vA4CdTUyS5o27dWrORaW1fu4f+9unbAR5qpUN+WB
TNSspN1zbpnjTb2/piQ3AMe7/PujMW5J7QyRUnrMc2LuhPjsB+6ySiymE9glgn2Q
EoGUJTKRLUIHM9CZezZm9dX7u0CMBK++MJrIUEJuzz2PamgT5KFGLMqpG19BUsXy
P8bcq90CgYEAvggvZJ51HNz9/jJR8AumOLeI5c2kGOgvZ1IUY2duugWzS5Ifq2/e
AZZR1YI4ICfYRzdMuC1mR4IIweie0qQ0NXKNKV4FK9D9qYuTKcwECh2eSxHrDpfr
iilQRvqdDpZoqeupTQT6T8x1ioYLiqP4xhTcwTtAWcBf0nNVY8P1VeMCgYEAt3Sy
3SPvh+FeZ+bkEA3j5t2JlNItzUObBFH9tuGCn1L9skxhHBa0WU8WtltFFOg1my0C
ovTK5JgsV8rEhCgX77SJFpw9KfPuywlEmpEwwWFlmNMH3Bo/Oli2BtYSlQm3P1YS
MwiKOGvDe5x2TugcylWBvFvrZA3EaOsxTq5jgiMCgYAQCmjVuXpxQUY/WsgH1zPk
vblUjJM5dv/pXMhTy7tRAIIt8tuxgPPgdmvZ9e/+5t4exNA5C7wCdTuAAr7FUQ8H
I/A+ea0+EZoX4aw+bvY4VF+0PoL07GX5d5tLIoR3YsLa9QWJds4aRkv1sXLv9P9e
2Qv5U6MO8GomPY4LqdLfuQKBgQCFf0wxE9dYH62dGi1XaWQwOYHGY/OkbM+/IxLn
I87ztWabem0oDXWLj7vNUzk/74Iiu2wxvMRv+DQ8Znb+XFaevdcMNwFjJ/jakpHz
kDfUYNWXkekkR40/HhLHgrve7EUGt9FihZJfYDvOFZsZ7FoW+oX/1TvDgSkt2Af3
Y3tR1QKBgF23Hdxa765/eNVDf3/XS7gHez3P0USdgPePMY+4ZUCR1R7Ny7vF+Jor
CzzjFSJgrSdpaNdP8DshyMcMxjY3V1WBF3eRQCwjK3zDLdEnDkTPkQ+SQfr1WbII
MWr+qAJfVZJ5gj/Aj/qL7wrUr+ZgJB18A5rkgtXAnRgrJtjzAa6a
-----END RSA PRIVATE KEY-----
EOF;

$rsa2048_encrypted_private = <<<EOF
-----BEGIN ENCRYPTED PRIVATE KEY-----
MIIFLTBXBgkqhkiG9w0BBQ0wSjApBgkqhkiG9w0BBQwwHAQI9ZipXwiu1eACAggA
MAwGCCqGSIb3DQIJBQAwHQYJYIZIAWUDBAEqBBAZBPJGXd736eg59bX7iBwzBIIE
0Db72xL+6C55vgDpCTBsoZA98CSexuCxVKOlBHBh/4BbAzptBofSHXFEaxfO2tMN
/aXoWUhMHMB5z3nGhVoPiN08lKbuR7FrKkk/19uy6Z0iVN399AYpC9qg3NURXLt7
q5PUDfHCficjecTdbHFc2lNZmtow7nGCS9ngZ9gD4JnHhCniye50r+W0YSg7CpCj
Qr5vcBQSykleEogQr1V1+ny7VaMCcXFmSNhi+KpTSdWLEx67nyYjHTdlP3X3gH5a
tMT96NXKi1fvWFh3I3eRfJZfTh6hdZhhK8RUGuwJRe6gY2btCr5GBlpPSjfbnSMe
CvTt6Zhgnz4lcmU9MTN6mLtRlhL8xfzCzdLYXD08MPCxiA9l8jPIMVIfAt2VOTPr
dlQHn66YIIj4R+CgykhOeyPCgL1GkkkNWdnI6OUO8vGObaSfiAVhLytVJx/A4tka
ueEgVhPk9RLRXGyRUpe0WWsgwPqR3RrPAs/Lm29YDpQdwXnLxgawToRtO0bgNdOk
Tl+7anR80QnGUjGCujgn173ANGD6eHAGuRmgdTWruQG6hjJraF/YfTiIBK9JL0K5
WspS1AplNCEO5umg4LYy/l4GlY7cweTe3nANCSakXKACQMfn0gHcXfjBhIMo2/hn
d2eLY+QxZKTTcV2F6mIUFTNoUSfmLJkBgc4JY5EFhU+QNAbS3437vXKIeoorEWmW
eSfBbNkd8Kq7/XQT7fZ6uUj9fiu1uppWLQ+zv7x2ifS67Rd3WQMs0CGkAeMZeaLy
9sWAzfKHHZxfVeJAbGqMeaekZYnK4FM2EHZrln0Ojkb0vdtBK4VcOvgK+RNr1Xee
fevoux9fwMNHN2R9HgDbRydLtfh2SbnqU3Iy0XedrcrMeFy6L8+OZTMTakqr7FyK
yszWP/aCsjDKjfVtDQZNv8nbKIqxnTyEUDWxNDxfg+JDP0dFbF4fuzvO3DiS1tPw
XyLcCDwlO+hg6+o09bXIJDRwE/t4yLAXENdpmVfbaXu44lRRKoEciVkHkpIOcWTC
/Q2BF7170ikuP7DqBcv+HNTAdhvV/WQIRct/noyeCQDM/TEvqNkcRzM8e73V03dy
fJujoyAsDpRnrZzBUECUSXeWNhvMITevwLASmKWCnKOG1HWgzcHkGgNw4AfAEGKi
WUV4QJqgf3aCof9S5gqHnJ7RfaV/FfoViiVt+K6+UOWkDpQUvyuEYKuYwRB/NqUw
NPNqxe2qAOgURE+ULbwgNDpxZXKLJ+KpmYUclp+jekOSOePulleLu7USjn3DKjNq
MR2sk0zE7ImdTPQqwM+g7xOA4d9lgW9iEnm9JF8/0iAOhmHwVNpMSyCLsMvy0lIU
KVtb8NmaV4Bq5Ifai/wGBmPLV03etktj1lRlShnGpn6uC/R4AQCa4UwA9m6rAzci
AZ7uw57PtYYu4Ru8r1td/sSQvqWEvEST0TMkGRaRdAfzDD18Rk2uAHV+h/g0NC2u
Lc6j9jdiK8XfIUXIBue+GEQaUEYQtL0MvixkE9IydiK0kDjKPMoN6Y0b3O0JiUkY
6eLsV2Zkr94Ng6FkExK+WMJzqr4TLnvsnHNSS2wh8eXVw205Aq9xxL9R+hw3V/aw
fFEP5BDUcBTsQe2SIuy2Rpcd2Rnq0smcrPGSj3BHPcWu
-----END ENCRYPTED PRIVATE KEY-----
EOF;

$rsa2048_certificate = <<<EOF
-----BEGIN CERTIFICATE-----
MIIDCzCCAfOgAwIBAgIUQM1Bj1CDfe7gIKFDaSIJlGtKur8wDQYJKoZIhvcNAQEL
BQAwFDESMBAGA1UEAwwJa3BocC10ZXN0MCAXDTI2MTEyNjA5MzQxMloYDzIxMjYx
MTAyMDkzNDEyWjAUMRIwEAYDVQQDDAlrcGhwLXRlc3QwggEiMA0GCSqGSIb3DQEB
AQUAA4IBDwAwggEKAoIBAQCILnpcP49RueO9K+c8zXxcFg08RzTDUb08lByhg3Nm
cESJJeqeOE82RK7pXipBV/wIck5JKLmCwmR7JLgdBEfvdCgoJGT6TTLI0yZgNkEo
Os+RnE3zc5EaOyzPQAcZBXpnh90izrgBTyp6za77iwnShL3/bBVbO5dNcXYxaIBF
d3gkHJLHZ1rsREh1vBy/b207rZbtNL6jYzYBFXvEm4iajF8uvZWItR9VQAssUNDD
fhLIp+DPEDKJZqeqBH8nDsJZOl36Nag5vRPFKVwPztthlNc+/Ee3hThf5raWb8fI
VSs7EkQksuZynOzpoSwnHWmnvtL7bZmvaeV4fJDg8QQJAgMBAAGjUzBRMB0GA1Ud
DgQWBBQrE8rPipwa5iUIQqf+iSG+nhuvizAfBgNVHSMEGDAWgBQrE8rPipwa5iUI
Qqf+iSG+nhuvizAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAj
8je+B0DUKwzWUyJVNH5Yxfv7yt9/ecgjjYkzAQWLBVmo2cdvbXI5XTs+uRP8Ve+c
JyDsaqTGK4WLDdVg6WgALEWVHSSMlQwq1kgJrC9BKJwUchetPBEP1sgbnwiIORrp
dY/SyVcJ6O4REor7ln/5TAx/vZsRqQpxPevqH+p6s0DcnGbLofkZwKrknLnIYuu2
HOSF0Xmep05KL7U/p5NE4uiu9P2oMrCmlXcNmzApmxDJFh9DCbnzeZ101Rxo82PC
ebbrcPpO7TkYmYEK1Hl3eQDlcfwoBbhzrFjOXu8416Vv8Ozq6pYmrRhX0S3yF5rZ
QAJKzdqREePp8H+ruKrn
-----END CERTIFICATE-----
EOF;

$other_rsa2048_private = <<<EOF
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDHQ8kvmP4yKEzI
kng0gMSexnVW5aiS7RDqMjU7gGxqDewH2Iu9meLPXup6BT21j2IiaX8Jlxv7beY2
AyYncMQiYrjo2Gesz+74tTzK+8Jy4sEvCXJNzhVDs1AQnENO3W7oahfqtNv3ziSp
ziwZTcWPWKRjEiMARMyn0aOmx5w+A79r+s3czUnmHITAfPhUC68oVq4duZqljdq/
O3KK+W+CAuO1fjrG6VJLiKVO/9MIZcaTOHERd9eCoMNhNcNuXgWwL0fuh+T3PgQC
IdmZ813qLNIIsbkjNnVN4oDZCMY9ly1ZZB2ATycmXVdjMb7j0yCPYwtqyeqyBSUB
SlU+kLkxAgMBAAECggEAFl7UT80nwH7Iu/3Bk/de24phMvu+dJw7uWcglckBothk
Rq83ZPZJQcmgnklHV8I8nKtmUS/POudoIGkzF9tvkYKuyqk83mhLpk1GgW1JM7ek
Q4EWfYik14BifTLl1Y1DfcEnZTift07Dy/Jy/fY+tyyW6+5l0opmxXJiIn+nWUjE
fveOgTb5tZHVE9RPL5uo6SriWkGA7rGnw9IkRAyKrX2yKFxreqzqSvPN5QUG69e3
ZSa1qFW9aVH2HDyyx5lNF6M9Qp9FQ2HSPJMVYZksG+4EmGejEH317VGIDxr46Tkf
YagcejOSWiE8RRjLFM4wcWF54goB4KPDE39oO6PF4QKBgQD3zZtERsgofcSDAOqz
Tbr4TiV7vvzNOykTpPj/buooTx0NltHV0pMdosEtE0vXMfa+RzGSzGAJItfRAKq3
bQDtm0wm7PJqz831f0IIWpea5StHuDeUSZpFjLr3/EEgvF/bKjLfqCYHSnKhKIDv
8YvXNn6tRaDZyPg0drOe8zBoSQKBgQDN2yhABWpDXYnlnIUZcpDtPfR2XvOHCXiB
8xEgIrrBK2dsTtf2aMM3vkLogOlyixlXxaWoxZwad6z97AX1JwabGiWkXCcYiHEP
xHfRgIs/vJO193cDddYR5CREqRvrBGhF7U8x5iVzp4fhumbCeBsWxWa7uWfoXqae
fLBdbYbZqQKBgBBcY977KWX6/8mCevyBqlGQn0rvTjOir1CbuNDsmss0hf+KsKoE
cDApucVnG7/Gjbr8alp94e1YfpVPDPZ1XPVTJpD/XfYNaFTIJSz2BkJ01Qm0MP86
aNQkuYd8ATTPGykWPSulJ3Wdxbm8Od8dvJTmBTEdgsQ+T5zvJWbeOZj5AoGAcmNi
lVis+KwEkVuNNlvMHph7vD788b+YgBVSIdpKjkaRHMUfCmSifdoykcH4dtbGiVhh
uku8d5SiwyKhGN3LtOotmow+iUpgERPqRLc6KXufVNEjCAA1qRFt+d1CjzCOxLSe
eiraCs9AGXWb3lohK2EbuhrHm37k9VJyKihPc6kCgYEA3jRVorUV+4FCMNJFIbrL
SsD3gjSLLuldv8Hx4oR0/1+1BPRWO4uXWVI7J86J8YAjUHMEVKyeqlA2wbXd2ufG
OYPLk4NbmlydGUb0e15tM3g7YAdZMQ6TmcniQSgNyCFxV+gDtK2f3Y8QtddKvq5u
HrUXOMZ3y5MyRP31c0JhT/E=
-----END PRIVATE KEY-----
EOF;

$other_rsa2048_public = <<<EOF
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAx0PJL5j+MihMyJJ4NIDE
nsZ1VuWoku0Q6jI1O4Bsag3sB9iLvZniz17qegU9tY9iIml/CZcb+23mNgMmJ3DE
ImK46NhnrM/u+LU8yvvCcuLBLwlyTc4VQ7NQEJxDTt1u6GoX6rTb984kqc4sGU3F
j1ikYxIjAETMp9GjpsecPgO/a/rN3M1J5hyEwHz4VAuvKFauHbmapY3avztyivlv
ggLjtX46xulSS4ilTv/TCGXGkzhxEXfXgqDDYTXDbl4FsC9H7ofk9z4EAiHZmfNd
6izSCLG5IzZ1TeKA2QjGPZctWWQdgE8nJl1XYzG+49Mgj2MLasnqsgUlAUpVPpC5
MQIDAQAB
-----END PUBLIC KEY-----
EOF;

$rsa1024_private = <<<EOF
-----BEGIN PRIVATE KEY-----
MIICdQIBADANBgkqhkiG9w0BAQEFAASCAl8wggJbAgEAAoGBALkRofhdgux16Wnm
JjVwMqje2IElElA/1Zx863OMbqRV/ha5Fbzdr6HAwz1KfBL5w8nUGWYm1P97zdc+
jyOF+W64IidBfFpGGizA88JSY7U2EmlCNiWEzC/cwC2FT9rdXz4ewdRZE84byjI6
Hebepr2rFRLOeRiU3fVlPT3wPHZHAgMBAAECgYBuD5VyvjIBbDIvH4czmpEWKsk5
LJHSldP0p7ni0u5t3xIn81SBKJmtqZSy25wkMNOGMRQV0xwzAC6vRm4b4y4t7apq
BlZWO1Az2aTLxKEvvoSharaCo9JjXQAfs0Cq05G4GCTljUDsQo4TA00D7goK/VUn
yFemERws+rEhqlCp8QJBAOviMwWXsnA/dtlpOsn3IYnMgbzsb6YB7qOZkSTKQ6QZ
AJ4VI0BbGFvbdebP3a3KF+rjrS+7GlUm9dIhXLcfUj8CQQDI2gx3Uib3pTyie/VM
vPh1hhIWb1uRIfkvnfPWlbjTXrdLDrgjBiqs+xSRd5eUEePYWaqhTQxfUelScx/U
Tsn5AkAwb10Mq+5+7/kmgV4J9I3VbsHBGkd+rezjePf9tu6Wtm+TBkIbx9jDm9ek
0k/xjIrGt4NrRp8Mlxho0mLoUy0fAkAezBh3P1iMRoDqpTkywBMU6i7h08dllKEB
5SVbdO+3MWHHl4UykaJ/jaxUtdK/OJMIPI+GViscfpHZO5/YzQ/5AkAd15oqfu38
038srei2aEp2LuNMo7wmA+7sfZ6FoONSSaKbQ//XPgOPA0r78DBv3z4hj+Zkcszg
qKy62XKz0FsU
-----END PRIVATE KEY-----
EOF;

$rsa1024_public = <<<EOF
-----BEGIN PUBLIC KEY-----
MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC5EaH4XYLsdelp5iY1cDKo3tiB
JRJQP9WcfOtzjG6kVf4WuRW83a+hwMM9SnwS+cPJ1BlmJtT/e83XPo8jhfluuCIn
QXxaRhoswPPCUmO1NhJpQjYlhMwv3MAthU/a3V8+HsHUWRPOG8oyOh3m3qa9qxUS
znkYlN31ZT098Dx2RwIDAQAB
-----END PUBLIC KEY-----
EOF;

$rsa4096_private = <<<EOF
-----BEGIN PRIVATE KEY-----
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
rvjBpEFRFDjoXz4y3NvLYVtaYxX0
-----END PRIVATE KEY-----
EOF;

$rsa4096_public = <<<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAnrvIQ0RUdzUpKW17QgXA
7Uv9njX80BZy71F+b6fZdin7ovspD5Ea6TlZTLgtOVjFJibvlDe93dWNAXyYOFc6
zyrcAIzb5yTYRDuOEc0OmBC+3Fx/xJOY763hqfMocHSTu0vS1n+wualL8zJVoiaA
ts+Dxj6yYn5PPoDUTJ9X5ZDkhBzl+I6WUBJqsA0X51u11WRqgHgm+VTxzKSq8ZO4
/1tvZpuWZTsz8kvW6HNSwYtT9QfV0PLosLsGFSvKRK1upoZhaZUcHlHw/PCXxY3b
l/cJvQsBN8L0b/jcm+XRecMLGnCsNw3CtfhJec/jCOZEndkwm7qpH5oAh8+NiGg7
436u1UyH2nmh7JRaTvnL2JDksKOrCb7kNNQHOVLmrAEMcaxfaWNzpkNTl0LE6TSK
em1lZt9PVn2khcL0dP7Q19RKFF4dn2EgvAwx53Kyk8H8E89IjPc6m4Gg3PAw6zh0
q1LEuulcdYrX6A9w1JzbRWd+RNk/niLmnZL9PTP5TTn+NGPovGu7uN+kMTvoNzin
3XkMjwfPDceMaTi/rqmfZap3tf1GjhyEKF/rvVURCtd4bR1hD/s2KVcUJCTf2Jyn
FTyGgCrnisfX2Z8hT9FPisomItaAM29m/Ybr3nxqPDatHZ4Uq+QBQyzfl1u1XR1R
RJ3kw0Mm/PVbCA6ly0Tnb6kCAwEAAQ==
-----END PUBLIC KEY-----
EOF;

$ec_p256_private = <<<EOF
-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg82ZlBoZJ5IMISckg
W8pFU1kqPMw5mJN7WRRP1fXhHAehRANCAARdTNhnBc8oHhCRGb+0TE8uiHoTuZCc
9Zim6zOMNBeQje4Tjr9rzgPG8MDYnS5x0YzG97gNPir1X+69Uxcltrqm
-----END PRIVATE KEY-----
EOF;

$ec_p256_public = <<<EOF
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEXUzYZwXPKB4QkRm/tExPLoh6E7mQ
nPWYpuszjDQXkI3uE46/a84DxvDA2J0ucdGMxve4DT4q9V/uvVMXJba6pg==
-----END PUBLIC KEY-----
EOF;

$ec_p384_private = <<<EOF
-----BEGIN PRIVATE KEY-----
MIG2AgEAMBAGByqGSM49AgEGBSuBBAAiBIGeMIGbAgEBBDBfNUtU0vhiu7mnMEJ1
ySJqEpvFlONgOgTlzr84OO+dPfNI5m5welQtoEY2IM03LjahZANiAARHYdXJptT8
gnY8HAQRzVNwBt+QUZuisIe8zVXu7Sd+Oqwyb6mFJzoeY7I84G5ISUgVwKuT+fRM
mIhGoi/0ijmtKQaFXOaKXU8fcwVle82uDS+ejbI5IxmiQWzHR8ehAgI=
-----END PRIVATE KEY-----
EOF;

$ec_p384_public = <<<EOF
-----BEGIN PUBLIC KEY-----
MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAER2HVyabU/IJ2PBwEEc1TcAbfkFGborCH
vM1V7u0nfjqsMm+phSc6HmOyPOBuSElIFcCrk/n0TJiIRqIv9Io5rSkGhVzmil1P
H3MFZXvNrg0vno2yOSMZokFsx0fHoQIC
-----END PUBLIC KEY-----
EOF;

// signatures of "Hello, world!" created by `openssl dgst -<algo> -sign` with $rsa2048_private
$external_signatures = [
    'sha1' => 'Q/BMKNY2/deI3kbY+N6/q4oz3sshtLVpbtmQRfLKJQFqimx6+zb6RQJuOaNdXuydzBV8kdRMCNsZqaRu9CbD+6QoBunGNspyXf7o2/HdkkueksqGmE9mqInUGprecSXDoRGQipRbRQ3J2iQM1jwVANni4H0DZSMmyITFYUPOnV4z2/lN62ad1DIbychL/bxQ2aYHg4t22FDlCCJuB39lr0fBhtVoY+T2b4pmvJxN1LZscnZCxtQEiidVMfIKbVrg4WMSOHGd5DBd1iDK8V1HTH24Ja4cc5rcQ/1IGJigjvVC3Ycev8GVx29diKTbLnSUMAQoO1ZICjmr2vOXk8XFZw==',
    'sha224' => 'B5bZ2aJTqAgTtjOufCM805v397qpLB4PXdUqTld92l5r37ViE2cVWv3abLc/ptkYhcc+beXpb7uUBhqBU3/+k73UFvVmDHq6bPvpbILYMMD5821a0v3ZkjUtzIu/sGMUfZbIyBsGioKGuyZfcJB8ov7ViYBhuOC56ZB+snPc0nSnoA4w4wQLfSji432hvdw6w7TBrGv54tfmxaAVeaEAm3jcGv1arr6P3IJx7U/uzw3v9qbzVa7y8yjeSL0R6S8pTxR70+bsWRTO5zp04XAmn7y0qKEEFT0xPJxDYeyVgDuX0lWej0OSabcAzNzfKy9CerAoqNvrQnGrRGoDUBoJ6w==',
    'sha256' => 'Lxb5D7SE/gPmOilS01kmb9Y/s54eJb8Qhq4Ck0oFS4HbDTPi1QB9zyZVn3mV5Oq4QeqBBkSRlTL3r5yjrK8q2U50JaZXczUKEFzMk3OFkcfH9615LrIo8bqDuEoUI2fSc7f5zKMb/nX4e5O6ghlgpvt7Fd6k/rXy0HcaS0YE8mDTBA0vAMoxpJ6lYHTqNeohMWdbRsW+9+HD0CFGly2WFzhKRQBRpLvAW/AjRP5Nh2TVDp0OQOKZgy1j+QbOT7d1TUZ8rlUe5b67wv/1qzOcMK2S9pF0th8zRLf9rPUk0aBqkyB38qvjv8bSlI4G96F9TEE1BKp/W7IxdMtoWFHOUw==',
    'sha384' => 'bJOV4D0MTBGBtBm4mF7JN5mZNVctM6nuFvYkMJvO4zARLVE5c8wsdOxE6X1nKsXYCTDv+DK1fZmvk2fsACbi6+Qau3YqovW3tgtD6XX+eBDPwV3jY4eqReqH+1imQXZGpM1RYGli8Py9b/WAOjkz4N5esTMEg+MUxwS2fTvJjZC/MHvcVTQ9nsxlo7iP3HSZ+iZlO3tehpu/YvV/90WN3X0fIqWZuQX4gKUHNbKJwYkYffJ8inU2hNuHIAmPhMHNhciJnrKRdi2TakbRT1toEk6c2Aq2dlOJuMBSEayU35OCoerq29G7A8zZVl32hSgU1NNguy/xQFLo5C2GIhlW9g==',
    'sha512' => 'Jp113e21zjAvX0zjeBVVFeqg8HtDlPSnpr9WHRBev+P/A3ZRBWTuXXJFaMtdqOIS1bZikxvmryOJsoqQ/H4SNl6fR2XusG74biJWJkijZ8GoIB3B0WVPC3lHxkNSQDOWACCzY0Zs6oVj3DdFRfBzFi289S0lN1DnLguarSY6HKViRlEmTKF03aOt4+OoGi+MGJKA2rZBqBigCRwElJ5ao5QVMZLbONoL1CXSmFvx1k9aCbTyViDiBDK95FK4DJr3GmpGyWAsN+6GAhhJcshxOQE5mhZAOehVo8SjQOB63AUj/IgwZWahMLE9SKxLe2VXOtmvJBsAMeQc2TO4Kf0Gdg==',
    'md5' => 'RT3IBn2nRzlggjikNdZLNrSu606NEjR5Obqze5LRaBuP843jhuOiOOpMpvGxWcyERbfnB/6GjyWvYYiJv5iAr7mpSkCdoiDwVInDRsuNC6QksSSCJqJTUCkA3GYoJNlOWIQ1OsJ7E0kBb+DfzBZdHhaPzFLHWIzkwo/Hw5TS6ZxJeiNWxO51yC8PJO2fGsx9hdQHxEoXHJPmic9fGRVOChdqM22v2e9n+u0kxqnWt3HbmaLs5LZT4rnNfuPi53oMIIgtaEs0zI0UoXILnDp3vq34nkao5EKryA0vbmxs1eYV9//LhufjgdAHNTwejAouPE4urVrhImcKieV0lOMqBw==',
    'ripemd160' => 'YSXK3D+wuMpA8RAdywna96w4YlO52etrekz4CnqrLIpMhRO+WsrG0Nw+DDKnA5HtfaQwSu3f+IeuFQ/WkEAF+GVUVWGgRK2FsoGTven4/JBW/Vf09rBbgL9c6ZLf/v16DQva7zfZRDNts4OtVoiSo17PJICHTo2ae9I03aPUuJkOxYOf0Ngo9Mznz+Z5jla1JMlCCMqisvwOqHuF5Dkh7P1JwkeMbChuvf5Mx2jq6Wqk1A5bi3/wH694ZKYfgU4tKbmNAbKucpUA3VhMaNTTp8TYyv//H4EFvAQA8iSYcLnRkRNmQMpSIratPVTStJp7Z85fVIys68R9pm6251pPIw==',
];

// the same data signed with sha256 by `openssl dgst` using other keys
$external_other_rsa2048_sha256 = 'O77/lQXxJSx0zNpFmDifOVxTwqR2LOCF3QnbJ2UrkkftmfrEhKVC/Dhw4WEz5WTrxJ0k9CcqeMzYuR+DZCfgMBeN619xcKyxeaEfGq5poJ2OITF1LRom+0xT7S/MBRkiCqilmthXsBg5/rvCYWd+dd3VeoqirpTE5roFO1uNty0imMfG5JWxBM6fcyr9+maw6yGx2KfHgoiC3zE5G0LadEA/TlewBAqr4UH/yMI8CAMogY7putb5+J4cL2+PbTzVTl5HYLG3rXwBi5xIHJM4zgR9a5X2RVONKz10iKWTDyjIayAW7zQ2SzTjDBDhehA5cUpQPckAmblib9ticXrKrg==';
$external_rsa1024_sha256 = 'Ow2XCi1VBz52FLTce58iCvWMEzF5TQ6z9KUn983K4Bti7G3lL5jLyr1HU3yRGY0Abnyc7kmgRq5c4A2RcfC8O5mE7g52N4d2Ao3d3V9h5hYVI6S3Cmpc2DBIfp/P0GAZ3pEQbCORFCITL6UwX+w2fP5d1m8GHZVgnq3WHlvM5+E=';
$external_rsa4096_sha256 = 'jn/yfc7zPBD/NwaBvTL4LCHvO05pPAph72gvyMfGRnSQ9hV71RRc72X5HLx5umwxGGOKZlZTuvCQtlabhsfL1kKqkrEtNc0gsvLwUHb4bu65ypB+sozB5bQBaf+0ZN+5OxOEX+50XmvaKim8bveVFy7SQZxqQ8DtHHi1xwLeztXRhwxhjF5f41JT7cguW5x4J/DnyIVU+sIOxuUZtne8rHDR6gwPgKW6xfrlFfQuHYIHk2iRE4kw+2jLBqwE18pjv1qCJDL7TlUJc1FzQRxJ1r22Anhtqnzkcsi/MFSfntLIWh9FDZXXNaEeXUNEt53xki+w7edJzzxsGSTnoNF6uexs9Xq4UCFCqBIcF7fcaH+TZkxUJQA1gDLDI+E1olIo3K+LPjl30OYd3mWiVET6Yzf+CP6zPaqgSzHiv6NDwbJ7KPEw+p8Vn92zGjnct0EbHbbfThPr1v66hlS3bgpjC4/Yz7mpjdLybzN/HDNrkDaDdEQMw16AFLK5mgonyUS7IJ7d8tBu1usSIzUXDkp38j3GFQbMNOsPefkZfQe+kimiHTAWGwjFW/BTkNTKre3Wu7RUChA2dWaG0bDq0cFd6YT0OLp2/Tog+KAeIkdURz1x6aiCJuCPIItQ6L7n6OlZFnjJ4XINFcRSIq/9f78NUmGIBU4UxrnmlsWzqbs/Qlk=';
$external_ec_p256_sha256 = 'MEQCICDq8rKVyYMK9+hQ3ILJOCUtc8IKuTkLpNd90zvSJAa0AiAgPeJF+TZtVrt1V1VZQiQXjfN7C0oNHrJr/Q++BPz0nA==';

const ALGORITHMS = [
    'sha1' => OPENSSL_ALGO_SHA1,
    'sha224' => OPENSSL_ALGO_SHA224,
    'sha256' => OPENSSL_ALGO_SHA256,
    'sha384' => OPENSSL_ALGO_SHA384,
    'sha512' => OPENSSL_ALGO_SHA512,
    'md5' => OPENSSL_ALGO_MD5,
    'ripemd160' => OPENSSL_ALGO_RMD160,
];

function all_bytes(): string {
    $res = '';
    for ($i = 0; $i < 256; ++$i) {
        $res .= chr($i);
    }
    return $res;
}

function sign_data(string $data, $key, int $algorithm = OPENSSL_ALGO_SHA1): string {
    $signature = '';
    if (!openssl_sign($data, $signature, $key, $algorithm)) {
        return 'SIGN FAILED';
    }
    return $signature;
}

function test_basic() {
    global $rsa2048_private, $rsa2048_public;

    $data = 'Hello, world!';
    $signature = '';
    var_dump(openssl_sign($data, $signature, $rsa2048_private));
    var_dump(strlen($signature));
    var_dump(base64_encode($signature));
    var_dump(openssl_verify($data, $signature, $rsa2048_public));
    var_dump(openssl_verify($data . '!', $signature, $rsa2048_public));
    var_dump(openssl_verify('', $signature, $rsa2048_public));
    var_dump(openssl_verify('hello, world!', $signature, $rsa2048_public));
}

function test_default_algorithm_is_sha1() {
    global $rsa2048_private, $rsa2048_public;

    $data = 'default algorithm';
    $default_signature = sign_data($data, $rsa2048_private);
    $sha1_signature = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA1);
    var_dump($default_signature === $sha1_signature);
    var_dump(openssl_verify($data, $default_signature, $rsa2048_public));
    var_dump(openssl_verify($data, $default_signature, $rsa2048_public, OPENSSL_ALGO_SHA1));
    var_dump(openssl_verify($data, $sha1_signature, $rsa2048_public));
    var_dump(openssl_verify($data, sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256), $rsa2048_public));
}

function test_all_algorithms() {
    global $rsa2048_private, $rsa2048_public;

    $data = 'Hello, world!';
    foreach (ALGORITHMS as $name => $algorithm) {
        var_dump($name);
        $signature = '';
        var_dump(openssl_sign($data, $signature, $rsa2048_private, $algorithm));
        var_dump(strlen($signature));
        var_dump(base64_encode($signature));
        var_dump(openssl_verify($data, $signature, $rsa2048_public, $algorithm));
        var_dump(openssl_verify($data . 'x', $signature, $rsa2048_public, $algorithm));

        // RSA PKCS#1 v1.5 signatures are deterministic
        var_dump(sign_data($data, $rsa2048_private, $algorithm) === $signature);
    }
}

function test_algorithm_mismatch() {
    global $rsa2048_private, $rsa2048_public;

    $data = 'algorithm mismatch';
    foreach (ALGORITHMS as $sign_name => $sign_algorithm) {
        $signature = sign_data($data, $rsa2048_private, $sign_algorithm);
        foreach (ALGORITHMS as $verify_name => $verify_algorithm) {
            $expected = $sign_name === $verify_name ? 1 : 0;
            var_dump(openssl_verify($data, $signature, $rsa2048_public, $verify_algorithm) === $expected);
        }
    }
}

function test_external_signatures() {
    global $rsa2048_private, $rsa2048_public, $external_signatures;

    $data = 'Hello, world!';
    foreach (ALGORITHMS as $name => $algorithm) {
        $signature = base64_decode($external_signatures[$name]);
        var_dump(strlen($signature));
        var_dump(openssl_verify($data, $signature, $rsa2048_public, $algorithm));
        var_dump(openssl_verify($data . ' ', $signature, $rsa2048_public, $algorithm));
    }

    // signatures made by openssl are byte-for-byte the same as ours
    foreach (ALGORITHMS as $name => $algorithm) {
        var_dump(sign_data($data, $rsa2048_private, $algorithm) === base64_decode($external_signatures[$name]));
    }
}

function test_key_sizes() {
    global $rsa1024_private, $rsa1024_public, $rsa2048_private, $rsa2048_public, $rsa4096_private, $rsa4096_public;
    global $external_rsa1024_sha256, $external_rsa4096_sha256;

    $data = 'Hello, world!';
    foreach ([[$rsa1024_private, $rsa1024_public], [$rsa2048_private, $rsa2048_public], [$rsa4096_private, $rsa4096_public]] as $pair) {
        foreach (ALGORITHMS as $algorithm) {
            $signature = '';
            var_dump(openssl_sign($data, $signature, $pair[0], $algorithm));
            var_dump(strlen($signature));
            var_dump(md5($signature));
            var_dump(openssl_verify($data, $signature, $pair[1], $algorithm));
        }
    }

    // signatures created by openssl
    var_dump(openssl_verify($data, base64_decode($external_rsa1024_sha256), $rsa1024_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, base64_decode($external_rsa4096_sha256), $rsa4096_public, OPENSSL_ALGO_SHA256));
}

function test_wrong_key() {
    global $rsa2048_private, $rsa2048_public, $other_rsa2048_private, $other_rsa2048_public;
    global $rsa1024_public, $rsa4096_public, $external_other_rsa2048_sha256;

    $data = 'wrong key';
    $signature = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256);
    var_dump(openssl_verify($data, $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $signature, $other_rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $signature, $rsa1024_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $signature, $rsa4096_public, OPENSSL_ALGO_SHA256));

    $other_signature = sign_data($data, $other_rsa2048_private, OPENSSL_ALGO_SHA256);
    var_dump($other_signature !== $signature);
    var_dump(openssl_verify($data, $other_signature, $other_rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $other_signature, $rsa2048_public, OPENSSL_ALGO_SHA256));

    var_dump(openssl_verify('Hello, world!', base64_decode($external_other_rsa2048_sha256), $other_rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify('Hello, world!', base64_decode($external_other_rsa2048_sha256), $rsa2048_public, OPENSSL_ALGO_SHA256));
}

function test_tampered_data() {
    global $rsa2048_private, $rsa2048_public;

    $data = 'The quick brown fox jumps over the lazy dog';
    $signature = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256);
    var_dump(openssl_verify($data, $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));

    $length = strlen($data);
    var_dump(openssl_verify('T' . substr($data, 1), $signature, $rsa2048_public, OPENSSL_ALGO_SHA256) === 1);
    var_dump(openssl_verify('t' . substr($data, 1), $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify(substr($data, 0, $length - 1) . 'G', $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify(substr($data, 0, 20) . 'X' . substr($data, 21), $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data . "\0", $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify("\0" . $data, $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify(substr($data, 1), $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify(substr($data, 0, $length - 1), $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify('', $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify(strtoupper($data), $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data . $data, $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
}

function test_tampered_signature() {
    global $rsa2048_private, $rsa2048_public;

    $data = 'tampered signature';
    $signature = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256);
    $length = strlen($signature);
    var_dump($length);

    // flip one bit in different parts of the signature
    foreach ([0, 1, 100, 127, 128, 200, $length - 2, $length - 1] as $position) {
        $tampered = substr($signature, 0, $position) . chr(ord($signature[$position]) ^ 1) . substr($signature, $position + 1);
        var_dump(strlen($tampered));
        var_dump(openssl_verify($data, $tampered, $rsa2048_public, OPENSSL_ALGO_SHA256));
    }

    // the values below are rejected either with 0 or with -1 depending on the OpenSSL version
    $rejected = [
        '',
        "\0",
        'a',
        str_repeat("\0", $length),
        str_repeat("\xff", $length),
        str_repeat('a', $length),
        substr($signature, 1),
        substr($signature, 0, $length - 1),
        substr($signature, 0, $length >> 1),
        $signature . "\0",
        $signature . $signature,
        "\0" . $signature,
        strrev($signature),
        $data,
    ];
    foreach ($rejected as $s) {
        var_dump(openssl_verify($data, $s, $rsa2048_public, OPENSSL_ALGO_SHA256) !== 1);
    }
}

function test_signature_of_other_data() {
    global $rsa2048_private, $rsa2048_public;

    $signature1 = sign_data('data 1', $rsa2048_private, OPENSSL_ALGO_SHA256);
    $signature2 = sign_data('data 2', $rsa2048_private, OPENSSL_ALGO_SHA256);
    var_dump($signature1 !== $signature2);
    var_dump(openssl_verify('data 1', $signature1, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify('data 2', $signature2, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify('data 1', $signature2, $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify('data 2', $signature1, $rsa2048_public, OPENSSL_ALGO_SHA256));
}

function test_data_variants() {
    global $rsa2048_private, $rsa2048_public;

    $variants = [
        '',
        'a',
        "\0",
        "\0\0\0\0",
        "a\0b",
        "\xff\xfe\xfd",
        'привет мир',
        "line1\nline2\r\n",
        all_bytes(),
        str_repeat('a', 55),
        str_repeat('a', 56),
        str_repeat('a', 64),
        str_repeat('a', 128),
        str_repeat('0123456789abcdef', 65536),
    ];
    foreach ($variants as $data) {
        foreach ([OPENSSL_ALGO_SHA1, OPENSSL_ALGO_SHA256, OPENSSL_ALGO_SHA512] as $algorithm) {
            $signature = '';
            var_dump(openssl_sign($data, $signature, $rsa2048_private, $algorithm));
            var_dump(strlen($signature));
            var_dump(md5($signature));
            var_dump(openssl_verify($data, $signature, $rsa2048_public, $algorithm));
        }
    }

    // data is hashed as a whole, NUL bytes do not terminate it
    var_dump(sign_data("a\0b", $rsa2048_private) !== sign_data('a', $rsa2048_private));
    var_dump(sign_data("a\0b", $rsa2048_private) !== sign_data('ab', $rsa2048_private));
    var_dump(openssl_verify('a', sign_data("a\0b", $rsa2048_private), $rsa2048_public));
}

function test_signature_out_param() {
    global $rsa2048_private, $rsa2048_public;

    // a previous value of the output variable is overwritten
    $signature = 'previous value';
    var_dump(openssl_sign('data', $signature, $rsa2048_private, OPENSSL_ALGO_SHA256));
    var_dump(strlen($signature));
    var_dump(openssl_verify('data', $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));

    $signature2 = str_repeat('x', 1000);
    var_dump(openssl_sign('data', $signature2, $rsa2048_private, OPENSSL_ALGO_SHA256));
    var_dump(strlen($signature2));
    var_dump($signature2 === $signature);

    // the same variable can be reused for several signatures
    $signature = '';
    foreach (['one', 'two', 'three'] as $data) {
        var_dump(openssl_sign($data, $signature, $rsa2048_private, OPENSSL_ALGO_SHA256));
        var_dump(openssl_verify($data, $signature, $rsa2048_public, OPENSSL_ALGO_SHA256));
    }
}

function test_private_key_formats() {
    global $rsa2048_private, $rsa2048_pkcs1_private, $rsa2048_encrypted_private, $rsa2048_public;

    $data = 'private key formats';
    $reference = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256);

    // PKCS#8 and PKCS#1 encodings of the same key give the same signature
    var_dump(sign_data($data, $rsa2048_pkcs1_private, OPENSSL_ALGO_SHA256) === $reference);

    $handle = openssl_pkey_get_private($rsa2048_private);
    var_dump($handle !== false);
    var_dump(sign_data($data, $handle, OPENSSL_ALGO_SHA256) === $reference);

    $pkcs1_handle = openssl_pkey_get_private($rsa2048_pkcs1_private);
    var_dump($pkcs1_handle !== false);
    var_dump(sign_data($data, $pkcs1_handle, OPENSSL_ALGO_SHA256) === $reference);

    $encrypted_handle = openssl_pkey_get_private($rsa2048_encrypted_private, 'secret123');
    var_dump($encrypted_handle !== false);
    var_dump(sign_data($data, $encrypted_handle, OPENSSL_ALGO_SHA256) === $reference);

    // the handle can be used many times
    for ($i = 0; $i < 5; ++$i) {
        var_dump(sign_data($data . $i, $handle, OPENSSL_ALGO_SHA256) === sign_data($data . $i, $rsa2048_private, OPENSSL_ALGO_SHA256));
    }

    // PEM with surrounding text and CRLF line endings
    var_dump(sign_data($data, "some text\n" . $rsa2048_private . "\ntrailing text", OPENSSL_ALGO_SHA256) === $reference);
    var_dump(sign_data($data, str_replace("\n", "\r\n", $rsa2048_private), OPENSSL_ALGO_SHA256) === $reference);

    var_dump(openssl_verify($data, $reference, $rsa2048_public, OPENSSL_ALGO_SHA256));
}

function test_public_key_formats() {
    global $rsa2048_private, $rsa2048_public, $rsa2048_certificate;

    $data = 'public key formats';
    foreach ([OPENSSL_ALGO_SHA1, OPENSSL_ALGO_SHA256, OPENSSL_ALGO_SHA512] as $algorithm) {
        $signature = sign_data($data, $rsa2048_private, $algorithm);

        // PEM public key
        var_dump(openssl_verify($data, $signature, $rsa2048_public, $algorithm));

        // X.509 certificate containing the public key
        var_dump(openssl_verify($data, $signature, $rsa2048_certificate, $algorithm));

        // handle of the public key
        $public_handle = openssl_pkey_get_public($rsa2048_public);
        var_dump($public_handle !== false);
        var_dump(openssl_verify($data, $signature, $public_handle, $algorithm));
        var_dump(openssl_verify($data . 'x', $signature, $public_handle, $algorithm));

        // handle created from the certificate
        $certificate_handle = openssl_pkey_get_public($rsa2048_certificate);
        var_dump($certificate_handle !== false);
        var_dump(openssl_verify($data, $signature, $certificate_handle, $algorithm));
    }

    // PEM with surrounding text and CRLF line endings
    $signature = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256);
    var_dump(openssl_verify($data, $signature, "some text\n" . $rsa2048_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $signature, str_replace("\n", "\r\n", $rsa2048_public), OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $signature, "some text\n" . $rsa2048_certificate, OPENSSL_ALGO_SHA256));

    // the handle is reusable
    $public_handle = openssl_pkey_get_public($rsa2048_public);
    for ($i = 0; $i < 5; ++$i) {
        $signature = sign_data("data $i", $rsa2048_private, OPENSSL_ALGO_SHA256);
        var_dump(openssl_verify("data $i", $signature, $public_handle, OPENSSL_ALGO_SHA256));
        var_dump(openssl_verify("data " . ($i + 1), $signature, $public_handle, OPENSSL_ALGO_SHA256));
    }
}

function test_invalid_private_keys() {
    global $rsa2048_private, $rsa2048_public, $rsa2048_certificate, $rsa2048_encrypted_private, $ec_p256_public;

    $signature = 'previous value';
    var_dump(openssl_sign('data', $signature, 'not a key'));
    var_dump(openssl_sign('data', $signature, ''));
    var_dump(openssl_sign('data', $signature, $rsa2048_public));
    var_dump(openssl_sign('data', $signature, $rsa2048_certificate));
    var_dump(openssl_sign('data', $signature, $ec_p256_public));
    var_dump(openssl_sign('data', $signature, "-----BEGIN PRIVATE KEY-----\nAAAA\n-----END PRIVATE KEY-----"));
    var_dump(openssl_sign('data', $signature, substr($rsa2048_private, 0, 100)));
    var_dump(openssl_sign('data', $signature, str_replace('MI', 'XX', $rsa2048_private)));

    // a handle of a public key is not a private key
    $public_handle = openssl_pkey_get_public($rsa2048_public);
    var_dump(openssl_sign('data', $signature, $public_handle));
    $certificate_handle = openssl_pkey_get_public($rsa2048_certificate);
    var_dump(openssl_sign('data', $signature, $certificate_handle));

    // a private key is not a valid public key
    var_dump(openssl_verify('data', 'signature', $rsa2048_private) !== 1);
    var_dump(openssl_verify('data', 'signature', 'not a key') !== 1);
    var_dump(openssl_verify('data', 'signature', '') !== 1);
    var_dump(openssl_verify('data', 'signature', "-----BEGIN PUBLIC KEY-----\nAAAA\n-----END PUBLIC KEY-----") !== 1);

    // a valid signature is not accepted with an invalid key
    $valid_signature = sign_data('data', $rsa2048_private);
    var_dump(openssl_verify('data', $valid_signature, $rsa2048_private) !== 1);
    var_dump(openssl_verify('data', $valid_signature, 'not a key') !== 1);
    var_dump(openssl_verify('data', $valid_signature, '') !== 1);

    $private_handle = openssl_pkey_get_private($rsa2048_private);
    var_dump(openssl_verify('data', $valid_signature, $private_handle) !== 1);
    var_dump(openssl_verify('data', $valid_signature, $rsa2048_public));
}

function test_forged_handles() {
    global $rsa2048_private, $rsa2048_public;

    $signature = sign_data('data', $rsa2048_private);

    // legacy runtime format
    foreach ([';;999999', '::999999', ';;abc', ';;', '::'] as $handle) {
        $result = '';
        var_dump(openssl_sign('data', $result, $handle));
        var_dump(openssl_verify('data', $signature, $handle) !== 1);
    }

    // K2 runtime format
    foreach (['P' . str_repeat("\xff", 8), 'S' . str_repeat("\xff", 8), 'P' . str_repeat("\0", 8), 'S' . str_repeat("\0", 8), 'P12345678', 'S12345678', 'P', 'S'] as $handle) {
        $result = '';
        var_dump(openssl_sign('data', $result, $handle));
        var_dump(openssl_verify('data', $signature, $handle) !== 1);
    }
}

function test_unknown_algorithm() {
    global $rsa2048_private, $rsa2048_public;

    $signature = sign_data('data', $rsa2048_private);
    $result = '';
    foreach ([999, 100, 1000000, -1, -100] as $algorithm) {
        var_dump(openssl_sign('data', $result, $rsa2048_private, $algorithm));
        var_dump(openssl_verify('data', $signature, $rsa2048_public, $algorithm) !== 1);
    }
}

function test_ec_keys() {
    global $ec_p256_private, $ec_p256_public, $ec_p384_private, $ec_p384_public;
    global $rsa2048_private, $rsa2048_public, $external_ec_p256_sha256;

    // ECDSA signatures are randomized, so only verification results are printed
    $data = 'Hello, world!';
    // MD5 is not used here: OpenSSL 1.1.1 (bundled into the legacy runtime) does not allow it for ECDSA, OpenSSL 3 does
    foreach ([[$ec_p256_private, $ec_p256_public], [$ec_p384_private, $ec_p384_public]] as $pair) {
        foreach ([OPENSSL_ALGO_SHA1, OPENSSL_ALGO_SHA224, OPENSSL_ALGO_SHA256, OPENSSL_ALGO_SHA384, OPENSSL_ALGO_SHA512] as $algorithm) {
            $signature = '';
            var_dump(openssl_sign($data, $signature, $pair[0], $algorithm));
            var_dump(strlen($signature) > 0);
            var_dump(strlen($signature) <= 150);
            var_dump(openssl_verify($data, $signature, $pair[1], $algorithm));
            var_dump(openssl_verify($data . 'x', $signature, $pair[1], $algorithm));
        }
    }

    // signature created by openssl
    var_dump(openssl_verify($data, base64_decode($external_ec_p256_sha256), $ec_p256_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data . 'x', base64_decode($external_ec_p256_sha256), $ec_p256_public, OPENSSL_ALGO_SHA256));

    // two signatures of the same data differ but both are valid
    $first = sign_data($data, $ec_p256_private, OPENSSL_ALGO_SHA256);
    $second = sign_data($data, $ec_p256_private, OPENSSL_ALGO_SHA256);
    var_dump($first !== $second);
    var_dump(openssl_verify($data, $first, $ec_p256_public, OPENSSL_ALGO_SHA256));
    var_dump(openssl_verify($data, $second, $ec_p256_public, OPENSSL_ALGO_SHA256));

    // keys of different types do not match
    $ec_signature = sign_data($data, $ec_p256_private, OPENSSL_ALGO_SHA256);
    $rsa_signature = sign_data($data, $rsa2048_private, OPENSSL_ALGO_SHA256);
    var_dump(openssl_verify($data, $ec_signature, $rsa2048_public, OPENSSL_ALGO_SHA256) !== 1);
    var_dump(openssl_verify($data, $rsa_signature, $ec_p256_public, OPENSSL_ALGO_SHA256) !== 1);
    var_dump(openssl_verify($data, $ec_signature, $ec_p384_public, OPENSSL_ALGO_SHA256) !== 1);

    // the same key handles
    $private_handle = openssl_pkey_get_private($ec_p256_private);
    $public_handle = openssl_pkey_get_public($ec_p256_public);
    var_dump($private_handle !== false);
    var_dump($public_handle !== false);
    $signature = sign_data($data, $private_handle, OPENSSL_ALGO_SHA384);
    var_dump(openssl_verify($data, $signature, $public_handle, OPENSSL_ALGO_SHA384));
    var_dump(openssl_verify($data . 'x', $signature, $public_handle, OPENSSL_ALGO_SHA384));
}

function test_sign_and_verify_loop() {
    global $rsa2048_private, $rsa2048_public;

    $private_handle = openssl_pkey_get_private($rsa2048_private);
    $public_handle = openssl_pkey_get_public($rsa2048_public);
    for ($i = 0; $i < 30; ++$i) {
        $data = "message $i";
        $signature = '';
        var_dump(openssl_sign($data, $signature, $private_handle, OPENSSL_ALGO_SHA256));
        var_dump(md5($signature));
        var_dump(openssl_verify($data, $signature, $public_handle, OPENSSL_ALGO_SHA256));
        var_dump(openssl_verify($data . '!', $signature, $public_handle, OPENSSL_ALGO_SHA256));
    }
}

test_basic();
test_default_algorithm_is_sha1();
test_all_algorithms();
test_algorithm_mismatch();
test_external_signatures();
test_key_sizes();
test_wrong_key();
test_tampered_data();
test_tampered_signature();
test_signature_of_other_data();
test_data_variants();
test_signature_out_param();
test_private_key_formats();
test_public_key_formats();
test_invalid_private_keys();
test_forged_handles();
test_unknown_algorithm();
test_ec_keys();
test_sign_and_verify_loop();
