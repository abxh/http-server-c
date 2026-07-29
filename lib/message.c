#include "message.h"

#include <ctype.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

Error_t tokenize_request_line_(const ErrorInfo_t ei, const strview LINE, struct RequestLine *out)
{
    RETURN_IF_NULL(ei, out);

    /*
       5.1 Request-Line
       The Request-Line begins with a method token, followed by the
       Request-URI and the protocol version, and ending with CRLF. The
       elements are separated by SP characters. No CR or LF is allowed
       except in the final CRLF sequence.

            Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    */
    strview SP1 = STRVIEW_EMPTY;
    if (!strview_find_first_char(LINE, ' ', &SP1)) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing first space delimiter in Request-Line"});
    }

    strview SP2 = STRVIEW_EMPTY;
    if (!strview_find_first_char(strview_drop(SP1, 1), ' ', &SP2)) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing second space delimiter in Request-Line"});
    }

    strview SLASH = STRVIEW_EMPTY;
    if (!strview_find_first_char(SP2, '/', &SLASH)) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing protocol name-version / delimiter"});
    }

    strview CLRS = STRVIEW_EMPTY;
    if (!strview_find_first(strview_drop(SLASH, 1), strview_from_cstr("\r\n"), &CLRS)) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing CLRS delimiter for Request-Line"});
    }

    // clang-format off
    out->method           = strview_take(LINE, (size_t)(SP1.buf - LINE.buf));
    out->url              = strview_take(strview_drop(SP1, 1), (size_t)(SP2.buf - (SP1.buf + 1)));
    out->protocol_name    = strview_take(strview_drop(SP2, 1), (size_t)(SLASH.buf - (SP2.buf + 1)));
    out->protocol_version = strview_take(strview_drop(SLASH, 1), (size_t)(CLRS.buf - (SLASH.buf + 1)));
    // clang-format on

    return NO_ERRORS;
}

Error_t tokenize_header_(const ErrorInfo_t ei, const strview LINE, struct HTTPHeader *out)
{
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
    strview COLON = STRVIEW_EMPTY;
    if (!strview_find_first_char(LINE, ':', &COLON)) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing `:` delimiter in header"});
    }

    strview FIELD_VALUE = strview_trim_left(COLON);
    strview CLRS = STRVIEW_EMPTY;
    if (!strview_find_first(strview_drop(FIELD_VALUE, 1), strview_from_cstr("\r\n"), &CLRS)) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "missing CLRS delimiter for header"});
    }

    // clang-format off
    out->field_name    = strview_take(LINE, (size_t)(COLON.buf - LINE.buf));
    out->field_content = strview_take(FIELD_VALUE, (size_t)(CLRS.buf - FIELD_VALUE.buf));
    // clang-format on

    return NO_ERRORS;
}

Error_t assemble_response_header_(
    const ErrorInfo_t ei,
    void *allocator_context,
    void *(*allocate)(void *context, size_t alignment, size_t size),
    struct StatusLine status,
    const strtable *headers,
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
        struct strview key;
        struct strview value;

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
        struct strview key;
        struct strview value;

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
