#include <iostream>
#include <map>
#include <vector>
#include <arpa/inet.h>
#include <sstream>
#include <algorithm>
#include <strings.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdio>
#include <sys/select.h>
#include <fstream>
#include <limits>

typedef std::map<std::string, std::string>	Headers;
typedef std::vector<std::string>			vs;

struct Request
{
	std::string	method;
	std::string	path;
	std::string query;
	std::string	version;
	Headers		headers;
	std::string	body;
};

struct Location
{
	std::string						path;
	vs								accepted_methods;
	std::pair<size_t,std::string>	redirect;
	std::string						root;
	std::string						index;
	bool							autoindex;
	bool							upload;
	std::string						upload_path;
};

typedef std::vector<Location>				Locations;

struct ServerCnf
{
	std::string					host;
	size_t						port;
	vs							server_names;
	size_t						client_max_body_size;
	Locations					locs;
};

class Response
{
	typedef bool (Response::*func)(Location const&, size_t);
	func		_req_func(std::string);
	
	private:
		static std::string	_httpVersion;
		int					_statusCode;
		std::string			_statusMsg;
		Headers				_headers;
		std::string			_body;
		Request				_req;
		ServerCnf			_srv;
		int					_pid;
		int					_fd;
		time_t				_timeout;

		size_t	_getValidServerCnf(std::vector<ServerCnf> const &, struct sockaddr_in const);
		size_t	_getValidLocation(Locations const &);
		
		bool	_handleGetRequest(Location const &, size_t);
		bool	_handlePostRequest(Location const &, size_t);
		bool	_handleDeleteRequest(Location const &, size_t);

		bool	_handleRegFile(std::string, struct stat);
		bool	_handleDir(std::string, struct stat, Location const&, size_t);
		bool	_fileListing(std::string&);
		bool	_handleCGI(Location const&, std::string, std::string);

		bool	_waitProc();
		bool	_readFromCGI();
		bool	_getCgiHeaders(std::string &);

		bool	_resRedir(size_t, size_t, std::string);
		
		bool	_resGenerate(size_t);
		bool	_resGenerate(size_t, size_t);
		bool	_resGenerate(size_t, std::string, struct stat);

	public:
		Response();
		Response(Response const &);
		Response &operator= (Response const &);
		~Response();

		bool build(Request const &, std::vector<ServerCnf> const &, struct sockaddr_in const);
		std::string toString();
};

std::string	specRes(size_t);
std::string	mimeType(std::string);
std::string statusMessage(size_t);

std::string utostr(size_t n);
size_t		strtou(std::string s);
std::string timeToStr(time_t clock);
char		**getCGIArgs(char*, char*, char*);
