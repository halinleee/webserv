#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include "type.hpp"
#include <iosfwd>
#include <string>
#include <vector>
#include <set>

enum HttpMethod
{
	METHOD_GET,
	METHOD_POST,
	METHOD_DELETE
};

class LocationConfig
{
	private:
		std::string root;
		std::string index;
		bool autoIndex;
		std::set<HttpMethod> methods;
		std::string uploadDir;
		std::string redirectPath;
		size_t redirectCode;
		std::string cgiExtension;
		std::string cgiPath;

	private:
		bool parseHttpMethod(const std::string& s, HttpMethod& out);
		bool parseLocationDir(std::vector<std::string> token);
	
	public:
		bool parseLocationBlock(std::ifstream &configFile);


	public:
		LocationConfig()
		{
			autoIndex = false;
			methods.insert(METHOD_GET); //메서드 추가할때 clear로 꼭 초기화
			redirectCode = STATUS_UNDEFINED;
		}

		const std::string& getRoot() const { return root; }
		const std::string& getIndex() const { return index; }
		const bool& getAutoIndex() const { return autoIndex; }
		const std::set<HttpMethod>& getMethods() const { return methods; }
		const std::string& getUploadDir() const { return uploadDir; }
		const std::string& getRedirectPath() const { return redirectPath; }
		const size_t& getRedirectCode() const { return redirectCode; }
		const std::string& getCgiExtension() const { return cgiExtension; }
		const std::string& getCgiPath() const { return cgiPath; }
};

#endif