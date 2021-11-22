#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

/* This is a simple example showing how to send mail using libcurl's IMAP
 * capabilities.
 *
 * Note that this example requires libcurl 7.30.0 or above.
 */

#define FROM_MAIL    "<kutukov@list.ru>"
#define TO_MAIL      "<kutukov@list.ru>"
#define CC_MAIL      "<kvit.vn@gmail.com>"

static const char *payload_text =
        "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n"
        "To: " TO_MAIL "\r\n"
        "From: " FROM_MAIL "(Example User)\r\n"
        "Cc: " CC_MAIL "(Another example User)\r\n"
        "Message-ID: "
        "<dcd7cb36-11db-487a-9f3a-e652a9458efd@rfcpedant.example.org>\r\n"
        "Subject: IMAP example message\r\n"
        "\r\n" /* empty line to divide headers from body, see RFC5322 */
        "Тестовое сообщение от curl.\r\n"
        "\r\n"
        "It could be a lot of lines, could be MIME encoded, whatever.\r\n"
        "Check RFC5322.\r\n";

struct upload_status {
    size_t bytes_read;
};

static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp)
{
    struct upload_status *upload_ctx = (struct upload_status *)userp;
    const char *data;
    size_t room = size * nmemb;

    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
        return 0;
    }

    data = &payload_text[upload_ctx->bytes_read];

    if(*data) {
        size_t len = strlen(data);
        if(room < len)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;

        return len;
    }

    return 0;
}

int main(void)
{
    CURL *curl;
    CURLcode res = CURLE_OK;

    curl = curl_easy_init();
    if(curl) {
        long infilesize;
        struct upload_status upload_ctx = { 0 };
        struct curl_slist *recipients = NULL;
        /* Set username and password */
        curl_easy_setopt(curl, CURLOPT_USERNAME, "kutukov");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "kvit2010");

        /* This will create a new message 100. Note that you should perform an
         * EXAMINE command to obtain the UID of the next message to create and a
         * SELECT to ensure you are creating the message in the OUTBOX. */
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.list.ru");

        /* If you want to connect to a site who is not using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you. */
#ifdef SKIP_PEER_VERIFICATION
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

        /* If the site you are connecting to uses a different host name that what
         * they have mentioned in their server certificate's commonName (or
         * subjectAltName) fields, libcurl will refuse to connect. You can skip
         * this check, but this will make the connection less secure. */
#ifdef SKIP_HOSTNAME_VERIFICATION
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

        /* Note that this option is not strictly required, omitting it will result
         * in libcurl sending the MAIL FROM command with empty sender data. All
         * autoresponses should have an empty reverse-path, and should be directed
         * to the address in the reverse-path which triggered them. Otherwise,
         * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
         * details.
         */
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM_MAIL);

        /* Add two recipients, in this particular case they correspond to the
         * To: and Cc: addressees in the header, but they could be any kind of
         * recipient. */
        recipients = curl_slist_append(recipients, TO_MAIL);
        recipients = curl_slist_append(recipients, CC_MAIL);
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        /* We are using a callback function to specify the payload (the headers and
         * body of the message). You could just use the CURLOPT_READDATA option to
         * specify a FILE pointer to read from. */
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* Since the traffic will be encrypted, it is very useful to turn on debug
         * information within libcurl to see what is happening during the
         * transfer */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        /* Send the message */
        res = curl_easy_perform(curl);

        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* Free the list of recipients */
        curl_slist_free_all(recipients);

        /* Always cleanup */
        curl_easy_cleanup(curl);
    }

    return (int)res;
}
