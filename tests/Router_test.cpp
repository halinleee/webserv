// Router 유닛-통합 테스트 하네스: root/alias 경로 리졸브 검증
//
// root와 alias는 location prefix를 다루는 방식이 다르다(nginx와 동일):
//   - alias: 매칭된 prefix를 잘라내고 alias 경로에 나머지를 붙인다.
//   - root : prefix를 자르지 않고 요청 경로 전체를 root 경로에 그대로 붙인다.
// 즉 location /static { root ./www; } 라면 GET /static/a.txt 는
// ./www/static/a.txt 를 가리킨다(./www/a.txt 가 아님). root 아래에
// location prefix와 같은 이름의 하위 디렉터리가 실제로 있어야 파일이 보인다.
//
// 진짜 conf 파일을 파싱해 ServerConfig/LocationConfig를 만들고
// Router::route()에 넣어 resolvedPath를 검증한다. 마지막 한 케이스는
// Handler::serve()까지 연결해 실제 200 응답이 나오는지 end-to-end로 확인한다.
//
// 빌드 예시:
//   c++ -Wall -Wextra -Werror -std=c++98 -I./include
//       tests/Router_test.cpp
//       src/http/Router.cpp src/http/Handler.cpp src/http/MultipartParser.cpp src/http/HttpUtils.cpp
//       src/config/ServerConfig.cpp src/config/LocationConfig.cpp src/utils/ConfigParseUtils.cpp
//       -o tests/Router_test
//   ./tests/Router_test

#include "Router.hpp"
#include "Handler.hpp"
#include "ServerConfig.hpp"
#include "RouteResult.hpp"
#include "Request.hpp"
#include "Response.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
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

static std::string g_fixtureRoot; // 테스트 fixture 루트(/tmp/router_test_<pid>)

static bool writeFile(const std::string& path, const std::string& content)
{
	std::ofstream ofs(path.c_str(), std::ios::binary);
	if (!ofs)
		return false;
	ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
	return ofs.good();
}

static void mkdirp(const std::string& path)
{
	std::string cur;
	for (size_t i = 1; i < path.size(); ++i)
	{
		if (path[i] == '/')
		{
			cur = path.substr(0, i);
			mkdir(cur.c_str(), 0755);
		}
	}
	mkdir(path.c_str(), 0755);
}

// conf 텍스트를 파일로 쓰고 파싱해 ServerConfig를 반환한다.
static bool buildServerConfig(const std::string& body, ServerConfig& out)
{
	std::string confPath = g_fixtureRoot + "/test.conf";
	std::string conf = "server 8080\n" + body + "end\n";
	if (!writeFile(confPath, conf))
		return false;

	std::ifstream ifs(confPath.c_str());
	if (!ifs.is_open())
		return false;

	std::string first;
	std::getline(ifs, first); // "server 8080" 라인 소비(ServerConfig는 그 아래부터 파싱)

	parseStatus st = out.parseServerConfigBlock(ifs);
	return st == PARSE_FILE_END || st == PARSE_SERVER_END;
}

static Request makeRequest(HttpMethod method, const std::string& path)
{
	Request req;
	req.method = method;
	req.path = path;
	return req;
}

// TC1: root는 location prefix를 자르지 않고 요청 경로 전체를 붙인다(nginx와 동일)
static void test_root_keeps_prefix()
{
	std::string root = g_fixtureRoot + "/tc1_root";
	mkdirp(root + "/static");
	writeFile(root + "/static/hello.txt", "hi");

	std::string conf = "\tlocation /static\n\t\troot " + root + "\n\t\tmethods GET\n";
	ServerConfig config;
	if (!buildServerConfig(conf, config))
	{
		report("TC1 root keeps prefix", false, "config parse failed");
		return;
	}

	Request req = makeRequest(METHOD_GET, "/static/hello.txt");
	RouteResult res = Router::route(config, req);

	std::string want = root + "/static/hello.txt";
	std::ostringstream d;
	bool ok = true;
	if (res.action != ACTION_STATIC) { ok = false; d << "action=" << res.action << " (want STATIC); "; }
	if (res.resolvedPath != want) { ok = false; d << "resolvedPath=[" << res.resolvedPath << "] want=[" << want << "]; "; }
	report("TC1 root keeps prefix (GET /static/hello.txt -> <root>/static/hello.txt)", ok, d.str());
}

// TC2: location "/" 은 prefix가 "/" 자체이므로 root와 그냥 이어붙인 것과 동일하게 보인다
static void test_root_at_slash_prefix()
{
	std::string root = g_fixtureRoot + "/tc2_root";
	mkdirp(root);
	writeFile(root + "/index.html", "<h1>hi</h1>");

	std::string conf = "\tlocation /\n\t\troot " + root + "\n\t\tindex index.html\n";
	ServerConfig config;
	if (!buildServerConfig(conf, config))
	{
		report("TC2 root at / prefix", false, "config parse failed");
		return;
	}

	Request req = makeRequest(METHOD_GET, "/index.html");
	RouteResult res = Router::route(config, req);

	std::string want = root + "/index.html";
	std::ostringstream d;
	bool ok = (res.action == ACTION_STATIC && res.resolvedPath == want);
	if (!ok) d << "action=" << res.action << " resolvedPath=[" << res.resolvedPath << "] want=[" << want << "]; ";
	report("TC2 root at / prefix -> <root>/index.html", ok, d.str());
}

// TC3: 중첩 prefix에서도 root는 전체 경로를 그대로 붙인다
static void test_root_nested_prefix()
{
	std::string root = g_fixtureRoot + "/tc3_root";
	mkdirp(root + "/a/b");
	writeFile(root + "/a/b/c.txt", "nested");

	std::string conf = "\tlocation /a/b\n\t\troot " + root + "\n";
	ServerConfig config;
	if (!buildServerConfig(conf, config))
	{
		report("TC3 root nested prefix", false, "config parse failed");
		return;
	}

	Request req = makeRequest(METHOD_GET, "/a/b/c.txt");
	RouteResult res = Router::route(config, req);

	std::string want = root + "/a/b/c.txt";
	std::ostringstream d;
	bool ok = (res.action == ACTION_STATIC && res.resolvedPath == want);
	if (!ok) d << "resolvedPath=[" << res.resolvedPath << "] want=[" << want << "]; ";
	report("TC3 root nested prefix (/a/b) -> <root>/a/b/c.txt", ok, d.str());
}

// TC4: alias는 root와 반대로 prefix를 잘라낸다 (대조군)
static void test_alias_strips_prefix()
{
	std::string aliasDir = g_fixtureRoot + "/tc4_alias";
	mkdirp(aliasDir);
	writeFile(aliasDir + "/x.txt", "aliased");

	std::string conf = "\tlocation /up\n\t\talias " + aliasDir + "\n";
	ServerConfig config;
	if (!buildServerConfig(conf, config))
	{
		report("TC4 alias strips prefix", false, "config parse failed");
		return;
	}

	Request req = makeRequest(METHOD_GET, "/up/x.txt");
	RouteResult res = Router::route(config, req);

	std::string want = aliasDir + "/x.txt";
	std::ostringstream d;
	bool ok = (res.action == ACTION_STATIC && res.resolvedPath == want);
	if (!ok) d << "resolvedPath=[" << res.resolvedPath << "] want=[" << want << "]; ";
	report("TC4 alias strips prefix (/up/x.txt -> <alias>/x.txt, no /up)", ok, d.str());
}

// TC5: end-to-end - root 아래에 prefix와 같은 이름의 하위 디렉터리를 실제로 만들어두면
// Router::route() + Handler::serve() 로 200과 실제 파일 내용이 나온다.
static void test_root_end_to_end_serves_file()
{
	std::string root = g_fixtureRoot + "/tc5_root";
	mkdirp(root + "/static");
	std::string content = "served via root";
	writeFile(root + "/static/ok.txt", content);

	std::string conf = "\tlocation /static\n\t\troot " + root + "\n\t\tmethods GET\n";
	ServerConfig config;
	if (!buildServerConfig(conf, config))
	{
		report("TC5 root end-to-end serve", false, "config parse failed");
		return;
	}

	Request req = makeRequest(METHOD_GET, "/static/ok.txt");
	RouteResult route = Router::route(config, req);
	Response res = Handler::serve(route, req);

	std::ostringstream d;
	bool ok = true;
	if (res.statusCode != STATUS_OK) { ok = false; d << "status=" << res.statusCode << " (want 200); "; }
	if (res.body != content) { ok = false; d << "body=[" << res.body << "] want=[" << content << "]; "; }
	report("TC5 root end-to-end -> 200 + correct body", ok, d.str());
}

// TC6: 흔한 오설정 함정 문서화 - root 바로 아래에 파일을 두면(하위 디렉터리 없이)
// prefix가 안 잘리므로 못 찾는다 -> 404. (alias였다면 200이었을 케이스)
static void test_root_misconfiguration_pitfall_404()
{
	std::string root = g_fixtureRoot + "/tc6_root";
	mkdirp(root);
	writeFile(root + "/plain.txt", "should not be found this way");

	std::string conf = "\tlocation /misuse\n\t\troot " + root + "\n\t\tmethods GET\n";
	ServerConfig config;
	if (!buildServerConfig(conf, config))
	{
		report("TC6 root misconfiguration pitfall", false, "config parse failed");
		return;
	}

	Request req = makeRequest(METHOD_GET, "/misuse/plain.txt");
	RouteResult route = Router::route(config, req);
	Response res = Handler::serve(route, req);

	std::ostringstream d; d << "status=" << res.statusCode << " resolvedPath=[" << route.resolvedPath << "]";
	report("TC6 root without matching subdir -> 404 (common misconfig)", res.statusCode == STATUS_NOT_FOUND, d.str());
}

int main()
{
	std::ostringstream rootss;
	rootss << "/tmp/router_test_" << getpid();
	g_fixtureRoot = rootss.str();
	mkdir(g_fixtureRoot.c_str(), 0755);

	std::cout << "=== Router root/alias resolve test ===\n";
	std::cout << "fixture root: " << g_fixtureRoot << "\n\n";

	test_root_keeps_prefix();             // TC1
	test_root_at_slash_prefix();          // TC2
	test_root_nested_prefix();            // TC3
	test_alias_strips_prefix();           // TC4
	test_root_end_to_end_serves_file();   // TC5
	test_root_misconfiguration_pitfall_404(); // TC6

	std::cout << "\n=== SUMMARY: " << g_pass << " passed, " << g_fail << " failed ===\n";
	if (g_fail > 0)
	{
		std::cout << "FAILURES:\n";
		for (size_t i = 0; i < g_failMsgs.size(); ++i)
			std::cout << "  - " << g_failMsgs[i] << "\n";
	}

	std::string rm = "rm -rf " + g_fixtureRoot;
	if (system(rm.c_str()) != 0)
		std::cout << "(warn) fixture cleanup failed: " << g_fixtureRoot << "\n";

	return g_fail == 0 ? 0 : 1;
}
