#include "Handler.hpp"
#include "HttpUtils.hpp"
#include "MultipartParser.hpp"

#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cctype>
#include <sstream>
#include <vector>

namespace
{
	std::string getHeaderCI(const std::map<std::string, std::string>& headers, const std::string& key)
	{
		for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		{
			if (it->first.size() != key.size())
				continue;

			bool match = true;
			for (size_t i = 0; i < key.size(); ++i)
			{
				if (std::tolower(static_cast<unsigned char>(it->first[i])) != std::tolower(static_cast<unsigned char>(key[i])))
				{
					match = false;
					break;
				}
			}
			if (match)
				return it->second;
		}
		return "";
	}

	std::string sanitizeUploadFilename(const std::string& filename)
	{
		size_t pos = filename.find_last_of("/\\");
		std::string base = (pos == std::string::npos) ? filename : filename.substr(pos + 1);
		if (base.empty() || base == "." || base == "..")
			return "";
		return base;
	}


	const int MAX_RESOLVE_ATTEMPTS = 1000;

	std::string insertSuffix(const std::string& p, const std::string& suffix)
	{
		size_t slash = p.find_last_of('/');
		std::string dir = (slash == std::string::npos) ? "" : p.substr(0, slash + 1);
		std::string filename = (slash == std::string::npos) ? p : p.substr(slash + 1);

		size_t dot = filename.find_last_of('.');
		std::string name;
		std::string ext;
		if (dot == std::string::npos || dot == 0)
		{
			name = filename;
			ext = "";
		}
		else
		{
			name = filename.substr(0, dot);
			ext = filename.substr(dot);
		}

		return dir + name + suffix + ext;
	}

	std::string resolveAvailablePath(const std::string& path, std::string* usedSuffixOut = NULL)
	{
		if (usedSuffixOut)
			*usedSuffixOut = "";

		struct stat st;
		if (stat(path.c_str(), &st) != 0)
			return path;

		for (int n = 1; n <= MAX_RESOLVE_ATTEMPTS; ++n)
		{
			std::ostringstream oss;
			oss << "(" << n << ")";
			std::string suffix = oss.str();
			std::string tmp = insertSuffix(path, suffix);
			if (stat(tmp.c_str(), &st) != 0)
			{
				if (usedSuffixOut)
					*usedSuffixOut = suffix;
				return tmp;
			}
		}
		return "";
	}

	std::string extractBoundary(const std::string& contentType)
	{
		std::string lowerType = contentType;
		for (size_t i = 0; i < lowerType.size(); ++i)
			lowerType[i] = std::tolower(static_cast<unsigned char>(lowerType[i]));

		size_t pos = lowerType.find("boundary=");
		if (pos == std::string::npos)
			return "";
		pos += 9;

		std::string rest = contentType.substr(pos);
		if (!rest.empty() && rest[0] == '"')
		{
			size_t end = rest.find('"', 1);
			if (end == std::string::npos)
				return "";
			return rest.substr(1, end - 1);
		}

		size_t semi = rest.find(';');
		if (semi != std::string::npos)
			rest = rest.substr(0, semi);
		return rest;
	}

	void setBody(Response& res, const std::string& body, const std::string& contentType)
	{
		res.body = body;
		res.headers["Content-Type"] = contentType;
	}

	bool readFile(const std::string& path, std::string& result, int& errOut)
	{
		int fd = open(path.c_str(), O_RDONLY);
		if (fd < 0)
		{
			errOut = errno;
			return false;
		}

		char buf[4096];
		ssize_t n;
		result.clear();
		while ((n = read(fd, buf, sizeof(buf))) > 0)
			result.append(buf, static_cast<size_t>(n));

		if (n < 0)
		{
			close(fd);
			return false;
		}
		close(fd);
		return true;
	}

	const std::string DEFAULT_ERROR_PAGE_PATH = "./www/error/default.html";

	std::string replaceAll(std::string s, const std::string& from, const std::string& to)
	{
		size_t pos = 0;
		while ((pos = s.find(from, pos)) != std::string::npos)
		{
			s.replace(pos, from.size(), to);
			pos += to.size();
		}
		return s;
	}

	enum WriteOutcome
	{
		WRITE_OK,
		WRITE_OPEN_FAILED,
		WRITE_IO_FAILED
	};

	WriteOutcome writeFile(const std::string& path, const std::string& data, int& errOut)
	{
		int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
		{
			errOut = errno;
			return WRITE_OPEN_FAILED;
		}

		size_t total = 0;
		while (total < data.size())
		{
			ssize_t n = write(fd, data.data() + total, data.size() - total);
			if (n < 0)
			{
				close(fd);
				return WRITE_IO_FAILED;
			}
			total += static_cast<size_t>(n);
		}
		close(fd);
		return WRITE_OK;
	}
}

Response Handler::checkErrno(int err)
{
	switch (err)
	{
		case ENOENT: return Response(STATUS_NOT_FOUND);
		case ENOTDIR: return Response(STATUS_NOT_FOUND);
		case EACCES: return Response(STATUS_FORBIDDEN);
		case EISDIR: return Response(STATUS_FORBIDDEN);
		default: return Response(STATUS_INTERNAL_SERVER_ERROR);
	}
}

Response Handler::buildAutoIndexPage(const std::string& dirPath, const std::string& reqPath)
{
	DIR* dir = opendir(dirPath.c_str());
	if (dir == NULL)
		return checkErrno(errno);

	std::string base = reqPath;
	if (base.empty() || base[base.size() - 1] != '/')
		base += "/";

	std::string escaped = HttpUtils::htmlEscape(base);

	std::ostringstream html;
	html << "<html><head><title>Index of " << escaped << "</title></head><body>\n";
	html << "<h1>Index of " << escaped << "</h1>\n<ul>\n";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		html << "<li><a href=\"" << HttpUtils::joinPath(escaped, HttpUtils::urlEncode(name)) << "\">" << HttpUtils::htmlEscape(name) << "</a></li>\n";
	}
	closedir(dir);

	html << "</ul>\n</body></html>\n";

	Response res(STATUS_OK);
	setBody(res, html.str(), "text/html");
	return res;
}

Response Handler::handleGet(const RouteResult& route, const std::string& reqPath)
{
	std::string path = route.resolvedPath;
	struct stat st;

	if (stat(path.c_str(), &st) != 0)
		return checkErrno(errno);

	if (S_ISDIR(st.st_mode))
	{
		bool foundIndex = false;

		if (!route.index.empty())
		{
			std::string indexPath = HttpUtils::joinPath(path, route.index);
			struct stat indexSt;
			if (stat(indexPath.c_str(), &indexSt) == 0 && S_ISREG(indexSt.st_mode))
			{
				path = indexPath;
				foundIndex = true;
			}
		}

		if (!foundIndex)
		{
			if (route.autoIndex)
				return buildAutoIndexPage(path, reqPath);
			return Response(STATUS_FORBIDDEN);
		}
	}
	else if (!S_ISREG(st.st_mode))
		return Response(STATUS_FORBIDDEN);

	std::string body;
	int err = 0;
	if (!readFile(path, body, err))
		return checkErrno(err);

	Response res(STATUS_OK);
	setBody(res, body, HttpUtils::getMimeType(path));
	return res;
}

Response Handler::handleDelete(const RouteResult& route)
{
	const std::string& path = route.resolvedPath;
	struct stat st;

	if (stat(path.c_str(), &st) != 0)
		return checkErrno(errno);

	if (!S_ISREG(st.st_mode))
		return Response(STATUS_FORBIDDEN);

	if (unlink(path.c_str()) != 0)
		return checkErrno(errno);

	return Response(STATUS_NO_CONTENT);
}

Response Handler::handlePost(const RouteResult& route, const Request& req)
{
	std::string contentType = getHeaderCI(req.headers, "Content-Type");
	bool isMultipart = HttpUtils::toLower(contentType).compare(0, 19, "multipart/form-data") == 0;

	if (!isMultipart)
	{
		const std::string& path = route.resolvedPath;

		size_t slash = path.find_last_of('/');
		if (slash != std::string::npos)
		{
			std::string parentDir = (slash == 0) ? "/" : path.substr(0, slash);
			struct stat dirSt;
			if (stat(parentDir.c_str(), &dirSt) != 0)
				return checkErrno(errno);
			if (!S_ISDIR(dirSt.st_mode))
				return Response(STATUS_NOT_FOUND);
		}

		struct stat targetSt;
		if (stat(path.c_str(), &targetSt) == 0 && S_ISDIR(targetSt.st_mode))
			return Response(STATUS_FORBIDDEN);

		std::string usedSuffix;
		std::string savePath = resolveAvailablePath(path, &usedSuffix);
		if (savePath.empty())
			return Response(STATUS_INTERNAL_SERVER_ERROR);

		int err = 0;
		WriteOutcome writeResult = writeFile(savePath, req.body, err);
		if (writeResult == WRITE_OPEN_FAILED)
			return checkErrno(err);
		if (writeResult == WRITE_IO_FAILED)
			return Response(STATUS_INTERNAL_SERVER_ERROR);

		Response res(STATUS_CREATED);
		res.headers["Location"] = usedSuffix.empty() ? req.path : insertSuffix(req.path, usedSuffix);
		return res;
	}

	struct stat dirSt;
	if (stat(route.resolvedPath.c_str(), &dirSt) != 0)
		return checkErrno(errno);
	if (!S_ISDIR(dirSt.st_mode))
		return Response(STATUS_FORBIDDEN);

	std::string boundary = extractBoundary(contentType);
	if (boundary.empty())
		return Response(STATUS_BAD_REQUEST);

	std::vector<MultipartPart> parts;
	if (!MultipartParser::parse(req.body, boundary, parts))
		return Response(STATUS_BAD_REQUEST);

	int savedCount = 0;
	std::string lastSavedUri;
	for (size_t i = 0; i < parts.size(); ++i)
	{
		if (parts[i].filename.empty())
			continue;

		std::string safeName = sanitizeUploadFilename(parts[i].filename);
		if (safeName.empty())
			continue;

		std::string filePath = HttpUtils::joinPath(route.resolvedPath, safeName);
		struct stat targetSt;
		if (stat(filePath.c_str(), &targetSt) == 0 && S_ISDIR(targetSt.st_mode))
			return Response(STATUS_FORBIDDEN);

		std::string usedSuffix;
		filePath = resolveAvailablePath(filePath, &usedSuffix);
		if (filePath.empty())
			return Response(STATUS_INTERNAL_SERVER_ERROR);

		int err = 0;
		WriteOutcome writeResult = writeFile(filePath, parts[i].data, err);
		if (writeResult == WRITE_OPEN_FAILED)
			return checkErrno(err);
		if (writeResult == WRITE_IO_FAILED)
			return Response(STATUS_INTERNAL_SERVER_ERROR);
		++savedCount;
		std::string savedName = usedSuffix.empty() ? safeName : insertSuffix(safeName, usedSuffix);
		lastSavedUri = HttpUtils::joinPath(req.path, savedName);
	}

	if (savedCount == 0)
		return Response(STATUS_BAD_REQUEST);

	Response res(STATUS_CREATED);
	if (savedCount == 1)
		res.headers["Location"] = lastSavedUri;
	return res;
}

Response Handler::serve(const RouteResult& route, const Request& req)
{
	switch (req.method)
	{
		case METHOD_GET: return handleGet(route, req.path);
		case METHOD_POST: return handlePost(route, req);
		case METHOD_DELETE: return handleDelete(route);
		default: return Response(STATUS_INTERNAL_SERVER_ERROR);
	}
}

Response Handler::buildErrorPage(Status code, const std::map<size_t, std::string>& errorPages)
{
	Response res(code);
	std::string body;
	int err = 0;

	std::map<size_t, std::string>::const_iterator it = errorPages.find(static_cast<size_t>(code));
	if (it != errorPages.end() && readFile(it->second, body, err))
	{
		setBody(res, body, HttpUtils::getMimeType(it->second));
		return res;
	}

	if (readFile(DEFAULT_ERROR_PAGE_PATH, body, err))
	{
		std::ostringstream codess;
		codess << static_cast<int>(code);
		body = replaceAll(body, "{{CODE}}", codess.str());
		body = replaceAll(body, "{{MESSAGE}}", res.statusText);
		setBody(res, body, "text/html");
		return res;
	}

	std::ostringstream codess;
	codess << static_cast<int>(code);
	setBody(res, "<html><body><h1>" + codess.str() + " " + res.statusText + "</h1></body></html>", "text/html");
	return res;
}
