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

// 버퍼에 raw 문자열을 한 번에 채워 넣고 parse()를 호출, 끝까지(REQ_PARSE_INCOMPLETE가 아닐 때까지) 반복
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

	// 요청 라인이 아직 다 안 들어옴
	appendStr(buf, "GET /index.h");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	// 요청 라인은 완성됐지만 헤더가 아직 안 옴
	appendStr(buf, "tml HTTP/1.1\r\n");
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	// 헤더 종료까지 도착
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
	// 공백이 하나뿐이라 method/target/version으로 분리 불가
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
	// Host 외에 헤더를 하나 둬서, 헤더 섹션 종료 검출과는 무관하게
	// "Host 누락" 검증 로직만 단독으로 테스트한다.
	ReqParseResult ret = feedAll(parser, buf, "GET / HTTP/1.1\r\nX-Foo: bar\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_zero_headers_request()
{
	RequestParser parser;
	CharDq buf;
	// 헤더가 하나도 없는 요청(start-line 뒤에 바로 빈 줄). Host가 없으니 400을 기대하지만,
	// parseHeaders가 헤더 섹션 종료를 findCRLFCRLF(4바이트 패턴)로만 찾기 때문에 남은 버퍼가
	// "\r\n"(2바이트)뿐인 이 경우를 끝까지 REQ_PARSE_INCOMPLETE로 본다. (버그)
	ReqParseResult ret = feedAll(parser, buf, "GET / HTTP/1.1\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_BAD_REQUEST);
}

static void test_header_section_too_large()
{
	RequestParser parser;
	CharDq buf;
	std::string bigValue(20000, 'a'); // MAX_HEADER_SECTION_LENGTH(16KB) 초과
	ReqParseResult ret = feedAll(parser, buf,
		"GET / HTTP/1.1\r\nHost: x\r\nX-Big: " + bigValue + "\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_HEADER_TOO_LARGE);
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
	// MAX_CLIENT_BODY_LENGTH(1000000) 초과
	ReqParseResult ret = feedAll(parser, buf,
		"POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 2000000\r\n\r\n");

	CHECK(ret == REQ_PARSE_ERROR);
	CHECK(parser.getRequest().status == STATUS_PAYLOAD_TOO_LARGE);
}

// 클라이언트가 요청을 다 보내지 않고 연결을 끊으면, 파서 입장에서는 그냥 더 이상
// 새 데이터가 도착하지 않는 상태가 된다. recv()가 0을 반환해도 Server는 onReceive()를
// 다시 호출하지 않지만, 혹시 다시 호출되더라도 크래시 없이 REQ_PARSE_INCOMPLETE를 유지하고
// 이미 받은 partial 데이터를 보존해야 한다(서버가 그 정보로 로그를 남기거나 정리할 수 있도록).
static void test_client_disconnect_mid_startline()
{
	RequestParser parser;
	CharDq buf;
	appendStr(buf, "GET /index.h"); // 요청 라인도 다 안 옴
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	size_t sizeBeforeRetry = buf.size();
	parser.parse(buf); // 연결이 끊겨 더 이상 데이터가 없는 상태에서 다시 호출돼도 안전해야 함
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);
	CHECK(buf.size() == sizeBeforeRetry);
}

static void test_client_disconnect_mid_headers()
{
	RequestParser parser;
	CharDq buf;
	appendStr(buf, "GET / HTTP/1.1\r\nHost: example.com\r\nX-Partial:"); // 헤더 줄이 끝나지 않음
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
		"hel"); // 10바이트 중 3바이트만 도착
	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);

	parser.parse(buf);
	CHECK(parser.getState() == REQ_PARSE_INCOMPLETE);
	CHECK(parser.getRequest().status == STATUS_UNDEFINED); // 에러로 처리되지 않아야 함
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
		"4\r\nWi"); // 청크 사이즈는 받았지만 본문이 덜 옴
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
