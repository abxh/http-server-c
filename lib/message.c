#include "message.h"

#include <ctype.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

Error_t tokenize_request_line_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct RequestLine *out)
{
    RETURN_IF_NULL(ei, line);
    RETURN_IF_NULL(ei, out);

    /*
       5.1 Request-Line
       The Request-Line begins with a method token, followed by the
       Request-URI and the protocol version, and ending with CRLF. The
       elements are separated by SP characters. No CR or LF is allowed
       except in the final CRLF sequence.

            Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    */
    const char *SP1 = strchr(line, ' ');
    if (!SP1) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing first space delimiter in Request-Line"});
    }

    const char *SP2 = strchr(SP1 + 1, ' ');
    if (!SP2) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing second space delimiter in Request-Line"});
    }

    const char *SLASH = strchr(SP2, '/');
    if (!SLASH) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing protocol name-version / delimiter"});
    }

    const char *CLRS = strstr(SLASH + 1, "\r\n");
    if (!CLRS) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing CLRS delimiter for Request-Line"});
    }

    if ((CLRS + 2) - line > (ptrdiff_t)line_len) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "unexpected line_len"});
    }

    // clang-format off
    out->method           = csview_with_n(line, SP1 - line);
    out->url              = csview_with_n(SP1 + 1, SP2 - (SP1 + 1));
    out->protocol_name    = csview_with_n(SP2 + 1, SLASH - (SP2 + 1));
    out->protocol_version = csview_with_n(SLASH + 1, CLRS - (SLASH + 1));
    // clang-format on

    return NO_ERRORS;
}

Error_t tokenize_header_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct HTTPHeader *out)
{
    RETURN_IF_NULL(ei, line);
    RETURN_IF_NULL(ei, out);

    /*
        4.2 Message Headers

           HTTP header fields, which include general-header (section 4.5),
           request-header (section 5.3), response-header (section 6.2), and
           entity-header (section 7.1) fields, follow the same generic format as
           that given in Section 3.1 of RFC 822 [9]. Each header field consists
           of a name followed by a colon (":") and the field value. Field names
           are case-insensitive. The field value MAY be preceded by any amount
           of LWS, though a single SP is preferred. Header fields can be
           extended over multiple lines by preceding each extra line with at
           least one SP or HT. Applications ought to follow "common form", where
           one is known or indicated, when generating HTTP constructs, since
           there might exist some implementations that fail to accept anything
           beyond the common forms.

               message-header = field-name ":" [ field-value ]
               field-name     = token
               field-value    = *( field-content | LWS )
               field-content  = <the OCTETs making up the field-value
                                and consisting of either *TEXT or combinations
                                of token, separators, and quoted-string>
    */
    const char *COLON = strchr(line, ':');
    if (!COLON) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing : delimiter in header"});
    }

    while (*(COLON + 1) == ' ') {
        COLON += 1;
    }

    const char *CLRS = strstr(COLON + 1, "\r\n");
    if (!CLRS) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing CLRS delimiter for header"});
    }

    if ((CLRS + 2) - line > (ptrdiff_t)line_len) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "unexpected line_len"});
    }

    // clang-format off
    out->field_name  = csview_with_n(line, COLON - line);
    out->field_value = csview_with_n(COLON + 1, CLRS - (COLON + 1));
    // clang-format on

    return NO_ERRORS;
}

Error_t assemble_response_header_(
    const ErrorInfo_t ei,
    void *allocator_context,
    void *(*allocate)(void *context, size_t alignment, size_t size),
    struct StatusLine status,
    struct csview_htable *headers,
    size_t *out_buf_len,
    char **out_buf)
{
    RETURN_IF_NULL(ei, allocate);
    RETURN_IF_NULL(ei, headers);
    RETURN_IF_NULL(ei, out_buf);

    /*
        6.1 Status-Line

       The first line of a Response message is the Status-Line, consisting
       of the protocol version followed by a numeric status code and its
       associated textual phrase, with each element separated by SP
       characters. No CR or LF is allowed except in the final CRLF sequence.

           Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    */

    /*
         HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
    */
    bool a1 = isdigit(status.http_version.buf[0]);
    bool a2 = status.http_version.buf[1] == '.';
    bool a3 = isdigit(status.http_version.buf[2]);
    if (status.http_version.size != 3 || !a1 || !a2 || !a3) {
        return error_format_location(
            ei,
            (Error_t){
                .tag = ERROR_CUSTOM,
                .custom_msg =
                    "http-version must be of the form <major>.<minor> with <major> and <minor> as single digits."});
    }

    bool b1 = isdigit(status.status_code.buf[0]);
    bool b2 = isdigit(status.status_code.buf[1]);
    bool b3 = isdigit(status.status_code.buf[2]);
    if (status.status_code.size != 3 || !b1 || !b2 || !b3) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "status code must be a 3-digit integer"});
    }

    size_t len = 0;
    len += sizeof("HTTP/X.X XXX ") - 1;
    len += (size_t)status.status_desc.size;
    len += 2; // CLRS
    {
        csview key;
        csview value;

        size_t idx;
        FHASHTABLE_FOR_EACH(headers, idx, key, value)
        {
            len += (size_t)key.size;
            len += 2; // ": "
            len += (size_t)value.size;
            len += 2; // CRLS
        }
    }
    len += 2; // CRLS

    *out_buf = (char *)allocate(
        allocator_context, alignof(char), sizeof(char) * (len + 1)); // +1 for c str functions printing '\0'
    *out_buf_len = len;

    if (out_buf == NULL) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = 12}); // OOM errno
    }

    size_t buf_idx = 0;
    snprintf(
        &(*out_buf)[buf_idx],
        len,
        "HTTP/%2s %3s %s\r\n",
        status.http_version.buf,
        status.status_code.buf,
        status.status_desc.buf);

    buf_idx += sizeof("HTTP/X.X XXX ") - 1;
    buf_idx += (size_t)status.status_desc.size;
    buf_idx += 2; // CLRS

    {
        csview key;
        csview value;

        size_t idx;
        FHASHTABLE_FOR_EACH(headers, idx, key, value)
        {
            snprintf(&(*out_buf)[buf_idx], len, "%s: %s\r\n", key.buf, value.buf);

            buf_idx += (size_t)key.size;
            buf_idx += 2; // ": "
            buf_idx += (size_t)value.size;
            buf_idx += 2; // CRLS
        }
    }

    snprintf(&(*out_buf)[buf_idx], len, "\r\n");

    return NO_ERRORS;
}
