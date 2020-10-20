#ifdef __cplusplus
extern "C" {
#endif

/* C version php base64_encode function */
/* returns 0: on success */
/* output contains zero terminated string base64 encoded string */
int base64_encode (const unsigned char* const input, int ilen, char *output, int olen);

/* returns negative value on error */
/* return os success number decoded bytes */
int base64_decode (const char *const input, unsigned char *output, int olen);

int base64url_encode (const unsigned char* const input, int ilen, char *output, int olen);

/* returns negative value on error */
/* return os success number decoded bytes */
int base64url_decode (const char *const input, unsigned char *output, int olen);

int base64_to_base64url (const char *const input, char *output, int olen);
int base64url_to_base64 (const char *const input, char *output, int olen);

#ifdef __cplusplus
}
#endif

