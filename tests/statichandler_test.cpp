// Handler / MultipartParser 유닛-통합 테스트 하네스
//
// Server.cpp 배선 전이므로 end-to-end curl로는 검증 불가.
// RouteResult / Request를 직접 구성해 Handler::serve()를 호출하고
// Response.statusCode / headers / body 및 실제 파일시스템 결과를 assert한다.
//
// 빌드 예시:
//   c++ -Wall -Wextra -Werror -std=c++98 -I./include
//       tests/Handler_test.cpp
//       src/http/Handler.cpp src/http/MultipartParser.cpp src/http/HttpUtils.cpp
//       -o tests/Handler_test
//   ./tests/Handler_test

#include "Handler.hpp"
#include "MultipartParser.hpp"
#include "RouteResult.hpp"
#include "Request.hpp"
#include "Response.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

// ------------------------------------------------------------------ 결과 집계

static int g_pass = 0;
static int g_fail = 0;
static std::vector<std::string> g_failMsgs;

static void report(const std::string& caseName, bool ok, const std::string& detail)
{
	if (ok)
	{
		++g_pass;
		std::cout << "[PASS] " << caseName << "\n";
	}
	else
	{
		++g_fail;
		std::cout << "[FAIL] " << caseName << " :: " << detail << "\n";
		g_failMsgs.push_back(caseName + " :: " + detail);
	}
}

// --------------------------------------------------------------- FS 유틸리티

static std::string g_root; // 테스트 fixture 루트

static bool writeFileFixture(const std::string& path, const std::string& content)
{
	std::ofstream ofs(path.c_str(), std::ios::binary);
	if (!ofs)
		return false;
	ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
	return ofs.good();
}

static bool readFileFixture(const std::string& path, std::string& out)
{
	std::ifstream ifs(path.c_str(), std::ios::binary);
	if (!ifs)
		return false;
	std::ostringstream ss;
	ss << ifs.rdbuf();
	out = ss.str();
	return true;
}

static bool fileExists(const std::string& path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

static bool isDir(const std::string& path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// ------------------------------------------------------------ 요청/라우트 빌더

static RouteResult makeStaticRoute(const std::string& resolved)
{
	RouteResult r;
	r.action = ACTION_STATIC;
	r.resolvedPath = resolved;
	r.autoIndex = false;
	return r;
}

static Request makeRequest(HttpMethod method, const std::string& path)
{
	Request req;
	req.method = method;
	req.path = path;
	return req;
}

static std::string headerOr(const Response& res, const std::string& key, const std::string& def)
{
	std::map<std::string, std::string>::const_iterator it = res.headers.find(key);
	if (it == res.headers.end())
		return def;
	return it->second;
}

// =================================================================== 테스트들

// GET 1: 존재하는 정적 파일 -> 200, Content-Type, Content-Length 일치
static void test_get_existing_file()
{
	std::string file = g_root + "/hello.html";
	std::string content = "<html><body>Hello Static</body></html>";
	writeFileFixture(file, content);

	RouteResult route = makeStaticRoute(file);
	Request req = makeRequest(METHOD_GET, "/hello.html");
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_OK) { ok = false; d << "status=" << res.statusCode << " (want 200); "; }
	if (res.body != content) { ok = false; d << "body mismatch; "; }
	std::string ct = headerOr(res, "Content-Type", "");
	if (ct != "text/html") { ok = false; d << "Content-Type=" << ct << " (want text/html); "; }
	std::ostringstream cl; cl << content.size();
	std::string clh = headerOr(res, "Content-Length", "");
	if (clh != cl.str()) { ok = false; d << "Content-Length=" << clh << " (want " << cl.str() << "); "; }
	report("GET-1 existing static file -> 200 + CT/CL", ok, d.str());
}

// GET 2: 존재하지 않는 파일 -> 404
static void test_get_missing_file()
{
	RouteResult route = makeStaticRoute(g_root + "/does_not_exist.html");
	Request req = makeRequest(METHOD_GET, "/does_not_exist.html");
	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("GET-2 missing file -> 404", res.statusCode == STATUS_NOT_FOUND, d.str());
}

// GET 3: 디렉터리 + index 존재 -> index 파일 내용으로 200
static void test_get_dir_with_index()
{
	std::string dir = g_root + "/withindex";
	mkdir(dir.c_str(), 0755);
	std::string content = "<h1>INDEX PAGE</h1>";
	writeFileFixture(dir + "/index.html", content);

	RouteResult route = makeStaticRoute(dir);
	route.index = "index.html";
	Request req = makeRequest(METHOD_GET, "/withindex/");
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_OK) { ok = false; d << "status=" << res.statusCode << "; "; }
	if (res.body != content) { ok = false; d << "body=[" << res.body << "]; "; }
	report("GET-3 directory with index -> 200 index content", ok, d.str());
}

// GET 4: 디렉터리 + index 없음 + autoIndex=true -> 200 + <li><a href=...>
static void test_get_autoindex_on()
{
	std::string dir = g_root + "/listing";
	mkdir(dir.c_str(), 0755);
	writeFileFixture(dir + "/alpha.txt", "a");
	writeFileFixture(dir + "/beta.txt", "b");

	RouteResult route = makeStaticRoute(dir);
	route.autoIndex = true;
	Request req = makeRequest(METHOD_GET, "/listing/");
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_OK) { ok = false; d << "status=" << res.statusCode << "; "; }
	if (res.body.find("<li><a href=") == std::string::npos) { ok = false; d << "no <li><a href=; "; }
	if (res.body.find("alpha.txt") == std::string::npos) { ok = false; d << "alpha.txt missing; "; }
	if (res.body.find("beta.txt") == std::string::npos) { ok = false; d << "beta.txt missing; "; }
	report("GET-4 autoindex on -> 200 HTML listing", ok, d.str());
}

// GET 5: 디렉터리 + index 없음 + autoIndex=false -> 403
static void test_get_autoindex_off()
{
	std::string dir = g_root + "/noindex";
	mkdir(dir.c_str(), 0755);
	writeFileFixture(dir + "/secret.txt", "x");

	RouteResult route = makeStaticRoute(dir);
	route.autoIndex = false;
	Request req = makeRequest(METHOD_GET, "/noindex/");
	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("GET-5 autoindex off -> 403", res.statusCode == STATUS_FORBIDDEN, d.str());
}

// GET 6: XSS 회귀 - 위험한 파일명이 이스케이프되어야 함
static void test_get_autoindex_xss_escape()
{
	std::string dir = g_root + "/xssdir";
	mkdir(dir.c_str(), 0755);
	// 주의: 원 시나리오의 `"><script>alert(1)</script>.txt` 는 `</script>` 안의 '/' 때문에
	// 리눅스 파일시스템에서 단일 파일로 생성 불가(ENOENT). 실제 저장형 XSS 벡터로 유효하면서
	// 파일시스템상 생성 가능한(슬래시 없는) 위험 파일명을 사용한다: 여전히 raw <, >, " 를 포함.
	std::string dangerous = "\"><script>alert(1).txt";
	std::string dpath = dir + "/" + dangerous;
	bool created = writeFileFixture(dpath, "payload");

	RouteResult route = makeStaticRoute(dir);
	route.autoIndex = true;
	Request req = makeRequest(METHOD_GET, "/xssdir/");
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (!created) { ok = false; d << "fixture file 생성 실패; "; }
	// raw <script> 태그가 body에 그대로 나오면 안 됨
	if (res.body.find("<script>") != std::string::npos) { ok = false; d << "raw <script> LEAKED (XSS!); "; }
	// 이스케이프된 형태가 존재해야 함
	if (res.body.find("&lt;script&gt;") == std::string::npos) { ok = false; d << "no &lt;script&gt; escaped form; "; }
	report("GET-6 autoindex XSS filename escaped", ok, d.str());
}

// POST 7: raw body, 신규 파일 -> 201 Created + 파일 생성 + 내용 일치
static void test_post_raw_new_file()
{
	std::string file = g_root + "/uploaded_new.txt";
	unlink(file.c_str());
	std::string content = "brand new raw content";

	RouteResult route = makeStaticRoute(file);
	Request req = makeRequest(METHOD_POST, "/uploaded_new.txt");
	req.body = content;
	// Content-Type 헤더 없음 -> raw

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_CREATED) { ok = false; d << "status=" << res.statusCode << " (want 201); "; }
	std::string onDisk;
	if (!readFileFixture(file, onDisk)) { ok = false; d << "file not created; "; }
	else if (onDisk != content) { ok = false; d << "content mismatch; "; }
	report("POST-7 raw new file -> 201 Created + written", ok, d.str());
}

// POST 8: 같은 경로 재 POST (중복 파일명) -> 덮어쓰지 않고 "(1)" 접미사로 새로 저장, 201 Created
static void test_post_raw_overwrite()
{
	std::string original = g_root + "/uploaded_new.txt"; // 7에서 이미 존재, 원본 내용 유지되어야 함
	std::string renamed = g_root + "/uploaded_new(1).txt";
	unlink(renamed.c_str());

	std::string originalContentBefore;
	readFileFixture(original, originalContentBefore);

	std::string content = "second upload, should not overwrite";

	RouteResult route = makeStaticRoute(original);
	Request req = makeRequest(METHOD_POST, "/uploaded_new.txt");
	req.body = content;

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_CREATED) { ok = false; d << "status=" << res.statusCode << " (want 201); "; }

	std::string originalAfter;
	if (!readFileFixture(original, originalAfter)) { ok = false; d << "original file missing; "; }
	else if (originalAfter != originalContentBefore) { ok = false; d << "original file was overwritten (should not be); "; }

	std::string renamedContent;
	if (!readFileFixture(renamed, renamedContent)) { ok = false; d << "renamed file uploaded_new(1).txt not created; "; }
	else if (renamedContent != content) { ok = false; d << "renamed file content mismatch; "; }

	std::map<std::string, std::string>::const_iterator loc = res.headers.find("Location");
	if (loc == res.headers.end()) { ok = false; d << "Location header missing; "; }
	else if (loc->second != "/uploaded_new(1).txt") { ok = false; d << "Location=" << loc->second << " (want /uploaded_new(1).txt); "; }

	report("POST-8 raw duplicate name -> 201 Created + renamed (1) + original kept", ok, d.str());
}

// POST 9: multipart/form-data, filename 파트 -> 201 + 파일 생성 + 내용 일치
static void test_post_multipart_upload()
{
	std::string dir = g_root + "/mpupload";
	mkdir(dir.c_str(), 0755);

	std::string boundary = "----testboundary9";
	std::string partContent = "multipart file body content";
	std::string body;
	body += "--" + boundary + "\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"upload9.txt\"\r\n";
	body += "Content-Type: text/plain\r\n";
	body += "\r\n";
	body += partContent + "\r\n";
	body += "--" + boundary + "--\r\n";

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/mpupload/");
	req.body = body;
	req.headers["content-type"] = "multipart/form-data; boundary=" + boundary;

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_CREATED) { ok = false; d << "status=" << res.statusCode << " (want 201); "; }
	std::string onDisk;
	if (!readFileFixture(dir + "/upload9.txt", onDisk)) { ok = false; d << "part file not created; "; }
	else if (onDisk != partContent) { ok = false; d << "content mismatch [" << onDisk << "]; "; }
	report("POST-9 multipart upload -> 201 + file saved", ok, d.str());
}

// POST 10: path traversal 회귀 - filename ../../../../tmp/pwned_XXXX
static void test_post_multipart_traversal()
{
	std::string dir = g_root + "/mptrav";
	mkdir(dir.c_str(), 0755);

	// 안전한 임의 suffix (진짜 시스템 파일 안 건드리도록 g_root 기반 임시 타깃 사용)
	std::ostringstream suf; suf << "pwned_" << getpid() << "_" << (rand() % 1000000);
	std::string evilBase = suf.str();
	// traversal 경로: 디렉터리 밖(g_root의 부모)을 노림. 실제로 안전한 임시 경로로 유도.
	std::string traversalTarget = g_root + "/OUTSIDE_" + evilBase; // dir 밖 (mptrav의 형제/상위)
	unlink(traversalTarget.c_str());

	std::string boundary = "----testboundary10";
	std::string partContent = "traversal payload";
	std::string body;
	body += "--" + boundary + "\r\n";
	// filename에 ../ 로 mptrav 디렉터리를 탈출하려는 시도
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"../OUTSIDE_" + evilBase + "\"\r\n";
	body += "\r\n";
	body += partContent + "\r\n";
	body += "--" + boundary + "--\r\n";

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/mptrav/");
	req.body = body;
	req.headers["content-type"] = "multipart/form-data; boundary=" + boundary;

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	// 디렉터리 밖에 파일이 생기면 안 됨
	if (fileExists(traversalTarget)) { ok = false; d << "ESCAPED! created outside dir: " << traversalTarget << "; "; }
	// basename(OUTSIDE_xxx) 으로 dir 안에 저장되어야 함
	std::string insidePath = dir + "/OUTSIDE_" + evilBase;
	if (!fileExists(insidePath)) { ok = false; d << "not saved as basename inside dir; "; }
	if (res.statusCode != STATUS_CREATED) { ok = false; d << "status=" << res.statusCode << " (want 201); "; }
	// 정리
	unlink(traversalTarget.c_str());
	report("POST-10 path traversal contained to basename", ok, d.str());
}

// POST 11: 깨진 multipart (boundary 없음) -> 400
static void test_post_multipart_broken()
{
	std::string dir = g_root + "/mpbroken";
	mkdir(dir.c_str(), 0755);

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/mpbroken/");
	req.body = "this body has no boundary markers at all";
	// Content-Type은 multipart지만 body에 boundary 구분자가 없음
	req.headers["content-type"] = "multipart/form-data; boundary=----nopresentboundary";

	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("POST-11 broken multipart -> 400", res.statusCode == STATUS_BAD_REQUEST, d.str());
}

// POST 16: 파트 데이터 뒤 CRLF 누락 (boundary 직전 CRLF 없음) -> 400
// RFC 7578: delimiter 앞에는 항상 CRLF가 와야 함. 이 CRLF가 없으면 malformed로 거부해야 한다.
static void test_post_multipart_missing_trailing_crlf()
{
	std::string dir = g_root + "/mpnocrlf";
	mkdir(dir.c_str(), 0755);

	std::string boundary = "----testboundary16";
	std::string partContent = "no trailing crlf before boundary";
	std::string body;
	body += "--" + boundary + "\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"upload16.txt\"\r\n";
	body += "\r\n";
	body += partContent; // 의도적으로 뒤에 "\r\n" 없이 바로 boundary로 이어붙임
	body += "--" + boundary + "--\r\n";

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/mpnocrlf/");
	req.body = body;
	req.headers["content-type"] = "multipart/form-data; boundary=" + boundary;

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_BAD_REQUEST) { ok = false; d << "status=" << res.statusCode << " (want 400); "; }
	if (fileExists(dir + "/upload16.txt")) { ok = false; d << "BUG: malformed part was still written to disk; "; }
	report("POST-16 multipart missing trailing CRLF -> 400, no file written", ok, d.str());
}

// POST 12: raw POST 대상 경로가 이미 존재하는 디렉터리 -> 403 Forbidden,
// resolveAvailablePath로 넘어가 "dirname(1)" 형제 파일을 몰래 생성하면 안 됨 (회귀 방지)
static void test_post_raw_target_is_directory()
{
	std::string dir = g_root + "/uploads_dir_target";
	mkdir(dir.c_str(), 0755);
	std::string sibling = g_root + "/uploads_dir_target(1)";
	unlink(sibling.c_str());

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/uploads_dir_target");
	req.body = "should not be written anywhere";

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_FORBIDDEN) { ok = false; d << "status=" << res.statusCode << " (want 403); "; }
	if (!isDir(dir)) { ok = false; d << "directory itself was modified/removed!; "; }
	if (fileExists(sibling)) { ok = false; d << "BUG: sibling file " << sibling << " was created; "; }
	report("POST-12 raw POST target is existing directory -> 403, no sibling file", ok, d.str());
}

// DELETE 13: 존재하는 파일 -> 204 + 실제 삭제
static void test_delete_existing()
{
	std::string file = g_root + "/to_delete.txt";
	writeFileFixture(file, "delete me");

	RouteResult route = makeStaticRoute(file);
	Request req = makeRequest(METHOD_DELETE, "/to_delete.txt");
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_NO_CONTENT) { ok = false; d << "status=" << res.statusCode << " (want 204); "; }
	if (fileExists(file)) { ok = false; d << "file still exists; "; }
	report("DELETE-13 existing file -> 204 + removed", ok, d.str());
}

// DELETE 14: 존재하지 않는 파일 -> 404
static void test_delete_missing()
{
	RouteResult route = makeStaticRoute(g_root + "/never_existed.txt");
	Request req = makeRequest(METHOD_DELETE, "/never_existed.txt");
	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("DELETE-14 missing file -> 404", res.statusCode == STATUS_NOT_FOUND, d.str());
}

// DELETE 15: 디렉터리 삭제 시도 -> 403
static void test_delete_directory()
{
	std::string dir = g_root + "/delete_dir";
	mkdir(dir.c_str(), 0755);

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_DELETE, "/delete_dir/");
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_FORBIDDEN) { ok = false; d << "status=" << res.statusCode << " (want 403); "; }
	if (!isDir(dir)) { ok = false; d << "directory was removed!; "; }
	report("DELETE-15 directory -> 403 (not deleted)", ok, d.str());
}

// ============================================================================

int main()
{
	// fixture 루트 생성
	std::ostringstream rootss;
	rootss << "/tmp/sh_test_" << getpid();
	g_root = rootss.str();
	mkdir(g_root.c_str(), 0755);

	std::cout << "=== Handler / MultipartParser test ===\n";
	std::cout << "fixture root: " << g_root << "\n\n";

	test_get_existing_file();          // 1
	test_get_missing_file();           // 2
	test_get_dir_with_index();         // 3
	test_get_autoindex_on();           // 4
	test_get_autoindex_off();          // 5
	test_get_autoindex_xss_escape();   // 6
	test_post_raw_new_file();          // 7
	test_post_raw_overwrite();         // 8
	test_post_multipart_upload();      // 9
	test_post_multipart_traversal();   // 10
	test_post_multipart_broken();      // 11
	test_post_multipart_missing_trailing_crlf(); // 16
	test_post_raw_target_is_directory(); // 12
	test_delete_existing();            // 13
	test_delete_missing();             // 14
	test_delete_directory();           // 15

	std::cout << "\n=== SUMMARY: " << g_pass << " passed, " << g_fail << " failed ===\n";
	if (g_fail > 0)
	{
		std::cout << "FAILURES:\n";
		for (size_t i = 0; i < g_failMsgs.size(); ++i)
			std::cout << "  - " << g_failMsgs[i] << "\n";
	}

	// fixture 정리 (best-effort)
	std::string rm = "rm -rf " + g_root;
	if (system(rm.c_str()) != 0)
		std::cout << "(warn) fixture cleanup failed: " << g_root << "\n";

	return g_fail == 0 ? 0 : 1;
}
