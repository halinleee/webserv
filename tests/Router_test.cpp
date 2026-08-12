
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

static std::string g_fixtureRoot;

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
	std::getline(ifs, first);

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

	test_root_keeps_prefix();
	test_root_at_slash_prefix();
	test_root_nested_prefix();
	test_alias_strips_prefix();
	test_root_end_to_end_serves_file();
	test_root_misconfiguration_pitfall_404();

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

