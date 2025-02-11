
#include "http_server.h"

const char RESPONSE_200_OK[] = "HTTP/1.0 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 17\r\n"
                               "\r\n"
                               "Hello Web from C!";
const char RESPONSE_400_BAD_REQUEST[] = "HTTP/1.0 400 Bad Request\r\n"
                                        "Content-Type: text/html\r\n"
                                        "Content-Length: 0\r\n"
                                        "\r\n";
const char RESPONSE_403_FORBIDDEN[] = "HTTP/1.0 403 Forbidden\r\n"
                                      "Content-Type: text/html\r\n"
                                      "Content-Length: 0\r\n"
                                      "\r\n";
const char RESPONSE_403_FORBIDDEN[] = "HTTP/1.0 404 Not Found\r\n"
                                      "Content-Type: text/html\r\n"
                                      "Content-Length: 0\r\n"
                                      "\r\n";
const char RESPONSE_500_INTERNAL_SERVER_ERROR[] = "HTTP/1.0 500 Internal Server Error\r\n"
                                                  "Content-Type: text/html\r\n"
                                                  "Content-Length: 0\r\n"
                                                  "\r\n";
const char RESPONSE_501_NOT_IMPLEMENTED[] = "HTTP/1.0 501 Not Implemented\r\n"
                                            "Content-Type: text/html\r\n"
                                            "Content-Length: 0\r\n"
                                            "\r\n";

Error_t tokenize_request_line_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct request_line *out)
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

Error_t tokenize_header_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct header *out)
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
