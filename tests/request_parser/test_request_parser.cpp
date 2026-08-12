#include "RequestParser.hpp"
#include <iostream>
#include <string>
#include <deque>
#include <sstream>

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond) \
	do { \
		if (cond) { ++g_pass; } \
		else { \
			++g_fail; \
			std::cerr << "  FAIL: " << #cond << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
		} \
	} while (0)

#define RUN(fn) \
	do { \
		std::cout << "[ " #fn " ]" << std::endl; \
		fn(); \
	} while (0)

static void appendStr(CharDq& buf, const std::string& s)
{
	buf.insert(buf.end(), s.begin(), s.end());
}

static ReqParseResult feedAll(RequestParser& parser, CharDq& buf, const std::string& raw)
{
	appendStr(buf, raw);
	parser.parse(buf);
	return parser.getState();
}

static void test_simple_get_request()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf,
		"GET /index.html HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"\r\n");

	CHECK(ret == REQ_PARSE_DONE);
	Request req = parser.getRequest();
	CHECK(req.method == METHOD_GET);
	CHECK(req.path == "/index.html");
	CHECK(req.host == "example.com");
	CHECK(req.port == 80);
	CHECK(req.contentLength == -1);
	CHECK(req.isChunked == false);
}

static void test_incomplete_then_complete()
{
	RequestParser parser;
	CharDq buf;

	appendStr(buf, "GET /index.h");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	appendStr(buf, "tml HTTP/1.1\r\n");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	appendStr(buf, "Host: example.com\r\n\r\n");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_DONE);

	Request req = parser.getRequest();
	CHECK(req.path == "/index.html");
	CHECK(req.host == "example.com");
}

static void test_post_with_content_length_body()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf,
		"POST /upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hello");

	CHECK(ret == REQ_PARSE_DONE);
	Request req = parser.getRequest();
	CHECK(req.method == METHOD_POST);
	CHECK(req.contentLength == 5);
	CHECK(req.body == "hello");
}

static void test_body_arrives_in_pieces()
{
	RequestParser parser;
	CharDq buf;

	appendStr(buf,
		"POST /upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Content-Length: 5\r\n"
		"\r\n"
		"hel");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	appendStr(buf, "lo");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_DONE);
	CHECK(parser.getRequest().body == "hello");
}

static void test_chunked_body()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf,
		"POST /upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\nWiki\r\n"
		"5\r\npedia\r\n"
		"0\r\n"
		"\r\n");

	CHECK(ret == REQ_PARSE_DONE);
	Request req = parser.getRequest();
	CHECK(req.isChunked == true);
	CHECK(req.body == "Wikipedia");
}

static void test_chunked_body_with_trailer_header()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf,
		"POST /upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\nWiki\r\n"
		"0\r\n"
		"X-Trailer: ignored\r\n"
		"\r\n");

	CHECK(ret == REQ_PARSE_DONE);
	CHECK(parser.getRequest().body == "Wiki");
}

static void test_bad_request_line()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "BADLINE\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_unsupported_method()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "PUT / HTTP/1.1\r\nHost: x\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_NOT_IMPLEMENTED);
}

static void test_unknown_method()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "FOO / HTTP/1.1\r\nHost: x\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_uri_too_long()
{
	RequestParser parser;
	CharDq buf;
	std::string longPath = "/" + std::string(5000, 'a');
	ReqParseResult ret = feedAll(parser, buf, "GET " + longPath + " HTTP/1.1\r\nHost: x\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_URI_LONG);
}

static void test_missing_host_header()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "GET / HTTP/1.1\r\nX-Foo: bar\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_zero_headers_request()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "GET / HTTP/1.1\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_header_section_too_large()
{
	RequestParser parser;
	CharDq buf;
	std::string bigValue(20000, 'a');
	ReqParseResult ret = feedAll(parser, buf,
		"GET / HTTP/1.1\r\nHost: x\r\nX-Big: " + bigValue + "\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_HEADER_TOO_LARGE);
}

static void test_header_count_too_many()
{
	RequestParser parser;
	CharDq buf;
	std::string req = "GET / HTTP/1.1\r\nHost: x\r\n";
	for (int i = 0; i < 101; ++i)
		req += "X-Extra: 1\r\n";
	req += "\r\n";
	ReqParseResult ret = feedAll(parser, buf, req);

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_HEADER_TOO_LARGE);
}

static void test_header_count_at_limit_ok()
{
	RequestParser parser;
	CharDq buf;
	std::string req = "GET / HTTP/1.1\r\nHost: x\r\n";
	for (int i = 0; i < 99; ++i)
		req += "X-Extra: 1\r\n";
	req += "\r\n";
	ReqParseResult ret = feedAll(parser, buf, req);

	CHECK(ret == REQ_PARSE_DONE);
}

static void test_dot_segment_path_rejected()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "GET /../etc/passwd HTTP/1.1\r\nHost: x\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_percent_decoding()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf, "GET /a%20b HTTP/1.1\r\nHost: x\r\n\r\n");

	CHECK(ret == REQ_PARSE_DONE);
	CHECK(parser.getRequest().path == "/a b");
}

static void test_content_length_and_transfer_encoding_conflict()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf,
		"POST / HTTP/1.1\r\n"
		"Host: x\r\n"
		"Content-Length: 5\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_payload_too_large()
{
	RequestParser parser;
	CharDq buf;
	ReqParseResult ret = feedAll(parser, buf,
		"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 2000000\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_PAYLOAD_TOO_LARGE);
}

static void test_client_disconnect_mid_startline()
{
	RequestParser parser;
	CharDq buf;
	appendStr(buf, "GET /index.h");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	size_t sizeBeforeRetry = buf.size();
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);
	CHECK(buf.size() == sizeBeforeRetry);
}

static void test_client_disconnect_mid_headers()
{
	RequestParser parser;
	CharDq buf;
	appendStr(buf, "GET / HTTP/1.1\r\nHost: example.com\r\nX-Partial:");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);
}

static void test_client_disconnect_mid_body()
{
	RequestParser parser;
	CharDq buf;
	appendStr(buf,
		"POST /upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Content-Length: 10\r\n"
		"\r\n"
		"hel");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);
	CHECK(parser.getRequest().status == STATUS_UNDEFINED);
}

static void test_client_disconnect_mid_chunked_body()
{
	RequestParser parser;
	CharDq buf;
	appendStr(buf,
		"POST /upload HTTP/1.1\r\n"
		"Host: example.com\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\nWi");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);
}

static void test_clear_resets_state_for_next_request()
{
	RequestParser parser;
	CharDq buf;

	ReqParseResult ret = feedAll(parser, buf, "BADLINE\r\n\r\n");
	CHECK(ret == REQ_PARSE_ERROR);

	parser.clear();
	buf.clear();

	ret = feedAll(parser, buf, "GET / HTTP/1.1\r\nHost: x\r\n\r\n");
	CHECK(ret == REQ_PARSE_DONE);
	CHECK(parser.getRequest().status == STATUS_UNDEFINED);
	CHECK(parser.getRequest().path == "/");
}

int main()
{
	RUN(test_simple_get_request);
	RUN(test_incomplete_then_complete);
	RUN(test_post_with_content_length_body);
	RUN(test_body_arrives_in_pieces);
	RUN(test_chunked_body);
	RUN(test_chunked_body_with_trailer_header);
	RUN(test_bad_request_line);
	RUN(test_unsupported_method);
	RUN(test_unknown_method);
	RUN(test_uri_too_long);
	RUN(test_missing_host_header);
	RUN(test_zero_headers_request);
	RUN(test_header_section_too_large);
	RUN(test_header_count_too_many);
	RUN(test_header_count_at_limit_ok);
	RUN(test_dot_segment_path_rejected);
	RUN(test_percent_decoding);
	RUN(test_content_length_and_transfer_encoding_conflict);
	RUN(test_payload_too_large);
	RUN(test_client_disconnect_mid_startline);
	RUN(test_client_disconnect_mid_headers);
	RUN(test_client_disconnect_mid_body);
	RUN(test_client_disconnect_mid_chunked_body);
	RUN(test_clear_resets_state_for_next_request);

	std::cout << std::endl << g_pass << " passed, " << g_fail << " failed" << std::endl;
	return g_fail == 0 ? 0 : 1;
}

