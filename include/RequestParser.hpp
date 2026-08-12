#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "type.hpp"
#include "Request.hpp"

const size_t MAX_STARTLINE_LENGTH = 8192;
const size_t MAX_URI_LENGTH = 4096;
const size_t MAX_HEADER_LINE_LENGTH = 1024;
const size_t MAX_HEADER_SECTION_LENGTH = 16 * 1024;
const size_t MAX_HEADER_COUNT = 100;
const size_t MAX_CLIENT_BODY_LENGTH = 1000000;
const size_t MAX_LEADING_BLANK_LINES = 5;

enum ParseState
{
	REQ_STARTLINE,
	REQ_HEADERS,
	REQ_BODY,
	REQ_DONE,
	REQ_ERROR
};

typedef struct
{
	std::string method;
	std::string target;
	std::string version;
} ReqLine;

class RequestParser
{
	private:
		ParseState parseState;

		ReqLine tmpReqLine;
		std::map<std::string, strVec> tmpHeaders;

		Request parsedReq;
		size_t chunkRemaining;
		bool inTrailer;
		size_t maxBodyLength;

		bool parseStartline(CharDq& buf);

		bool parseMethod(const std::string& method);

		bool parseURI(const std::string& target);

		bool parseVersion(const std::string& version);

		bool parseHeaders(CharDq& buf);
		bool validateHeaders();
		bool parseHost(const std::string& raw);
		bool validateContentLength(const strVec& cl);
		bool validateTransferEncoding(const strVec& te);
		void transferHeaders();

		bool parseBody(CharDq& buf);
		bool parsePlainBody(CharDq& buf);
		bool parseChunkedBody(CharDq& buf);

		void handleError();

	public:
		RequestParser();
		~RequestParser();

		void parse(CharDq& buf);
		void clear();
		void setMaxBodyLength(size_t length);

		ReqParseResult getState() const;
		Request getRequest() const;

		bool isIdle() const;
};

#endif