
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


static std::string g_root;

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

static void test_get_missing_file()
{
	RouteResult route = makeStaticRoute(g_root + "/does_not_exist.html");
	Request req = makeRequest(METHOD_GET, "/does_not_exist.html");
	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("GET-2 missing file -> 404", res.statusCode == STATUS_NOT_FOUND, d.str());
}

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

static void test_get_autoindex_xss_escape()
{
	std::string dir = g_root + "/xssdir";
	mkdir(dir.c_str(), 0755);
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
	if (res.body.find("<script>") != std::string::npos) { ok = false; d << "raw <script> LEAKED (XSS!); "; }
	if (res.body.find("&lt;script&gt;") == std::string::npos) { ok = false; d << "no &lt;script&gt; escaped form; "; }
	report("GET-6 autoindex XSS filename escaped", ok, d.str());
}

static void test_post_raw_new_file()
{
	std::string file = g_root + "/uploaded_new.txt";
	unlink(file.c_str());
	std::string content = "brand new raw content";

	RouteResult route = makeStaticRoute(file);
	Request req = makeRequest(METHOD_POST, "/uploaded_new.txt");
	req.body = content;

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_CREATED) { ok = false; d << "status=" << res.statusCode << " (want 201); "; }
	std::string onDisk;
	if (!readFileFixture(file, onDisk)) { ok = false; d << "file not created; "; }
	else if (onDisk != content) { ok = false; d << "content mismatch; "; }
	report("POST-7 raw new file -> 201 Created + written", ok, d.str());
}

static void test_post_raw_overwrite()
{
	std::string original = g_root + "/uploaded_new.txt";
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

static void test_post_multipart_traversal()
{
	std::string dir = g_root + "/mptrav";
	mkdir(dir.c_str(), 0755);

	std::ostringstream suf; suf << "pwned_" << getpid() << "_" << (rand() % 1000000);
	std::string evilBase = suf.str();
	std::string traversalTarget = g_root + "/OUTSIDE_" + evilBase;
	unlink(traversalTarget.c_str());

	std::string boundary = "----testboundary10";
	std::string partContent = "traversal payload";
	std::string body;
	body += "--" + boundary + "\r\n";
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
	if (fileExists(traversalTarget)) { ok = false; d << "ESCAPED! created outside dir: " << traversalTarget << "; "; }
	std::string insidePath = dir + "/OUTSIDE_" + evilBase;
	if (!fileExists(insidePath)) { ok = false; d << "not saved as basename inside dir; "; }
	if (res.statusCode != STATUS_CREATED) { ok = false; d << "status=" << res.statusCode << " (want 201); "; }
	unlink(traversalTarget.c_str());
	report("POST-10 path traversal contained to basename", ok, d.str());
}

static void test_post_multipart_broken()
{
	std::string dir = g_root + "/mpbroken";
	mkdir(dir.c_str(), 0755);

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/mpbroken/");
	req.body = "this body has no boundary markers at all";
	req.headers["content-type"] = "multipart/form-data; boundary=----nopresentboundary";

	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("POST-11 broken multipart -> 400", res.statusCode == STATUS_BAD_REQUEST, d.str());
}

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
	body += partContent;
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

static void test_post_multipart_target_is_directory()
{
	std::string dir = g_root + "/mpdirtarget";
	mkdir(dir.c_str(), 0755);
	std::string collidingDir = dir + "/collide";
	mkdir(collidingDir.c_str(), 0755);
	std::string sibling = dir + "/collide(1)";
	unlink(sibling.c_str());

	std::string boundary = "----testboundary17";
	std::string partContent = "should not be written anywhere";
	std::string body;
	body += "--" + boundary + "\r\n";
	body += "Content-Disposition: form-data; name=\"file\"; filename=\"collide\"\r\n";
	body += "Content-Type: text/plain\r\n";
	body += "\r\n";
	body += partContent + "\r\n";
	body += "--" + boundary + "--\r\n";

	RouteResult route = makeStaticRoute(dir);
	Request req = makeRequest(METHOD_POST, "/mpdirtarget/");
	req.body = body;
	req.headers["content-type"] = "multipart/form-data; boundary=" + boundary;

	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_FORBIDDEN) { ok = false; d << "status=" << res.statusCode << " (want 403); "; }
	if (!isDir(collidingDir)) { ok = false; d << "colliding directory itself was modified/removed!; "; }
	if (fileExists(sibling)) { ok = false; d << "BUG: sibling file " << sibling << " was created; "; }
	report("POST-17 multipart target is existing directory -> 403, no sibling file", ok, d.str());
}

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

static void test_delete_missing()
{
	RouteResult route = makeStaticRoute(g_root + "/never_existed.txt");
	Request req = makeRequest(METHOD_DELETE, "/never_existed.txt");
	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode;
	report("DELETE-14 missing file -> 404", res.statusCode == STATUS_NOT_FOUND, d.str());
}

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


int main()
{
	std::ostringstream rootss;
	rootss << "/tmp/sh_test_" << getpid();
	g_root = rootss.str();
	mkdir(g_root.c_str(), 0755);

	std::cout << "=== Handler / MultipartParser test ===\n";
	std::cout << "fixture root: " << g_root << "\n\n";

	test_get_existing_file();
	test_get_missing_file();
	test_get_dir_with_index();
	test_get_autoindex_on();
	test_get_autoindex_off();
	test_get_autoindex_xss_escape();
	test_post_raw_new_file();
	test_post_raw_overwrite();
	test_post_multipart_upload();
	test_post_multipart_traversal();
	test_post_multipart_broken();
	test_post_multipart_missing_trailing_crlf();
	test_post_raw_target_is_directory();
	test_post_multipart_target_is_directory();
	test_delete_existing();
	test_delete_missing();
	test_delete_directory();

	std::cout << "\n=== SUMMARY: " << g_pass << " passed, " << g_fail << " failed ===\n";
	if (g_fail > 0)
	{
		std::cout << "FAILURES:\n";
		for (size_t i = 0; i < g_failMsgs.size(); ++i)
			std::cout << "  - " << g_failMsgs[i] << "\n";
	}

	std::string rm = "rm -rf " + g_root;
	if (system(rm.c_str()) != 0)
		std::cout << "(warn) fixture cleanup failed: " << g_root << "\n";

	return g_fail == 0 ? 0 : 1;
}

