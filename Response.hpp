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
	typedef void (Response::*func)(Location const&, size_t);
	func		_req_func(std::string);
	
	private:
		static std::string	_httpVersion;
		int					_statusCode;
		std::string			_statusMsg;
		Headers				_headers;
		std::string			_body;
		int					_fd;
		Request				_req;
		ServerCnf			_srv;

		size_t	_getValidServerCnf(std::vector<ServerCnf> const &, struct sockaddr_in const);
		size_t	_getValidLocation(Locations const &);
		
		void	_handleGetRequest(Location const &, size_t);
		void	_handlePostRequest(Location const &, size_t);
		void	_handleDeleteRequest(Location const &, size_t);

		void	_handleRegFile(std::string, struct stat);
		void	_handleDir(std::string, struct stat, Location const&, size_t);
		void	_file_listing();
		void	_handleCGI(Location const&, std::string, std::string);

		void	_getCGIRes(int);
		int		_readFromCGI(int, std::fstream &, fd_set*, size_t&);
		bool	_getCgiHeaders();

		void	_resRedir(size_t, size_t, std::string);
		
		void	_resGenerate(size_t);
		void	_resGenerate(size_t, size_t);
		void	_resGenerate(size_t, int, std::string, struct stat);

	public:
		Response();
		Response(Response const &);
		Response &operator= (Response const &);
		~Response();

		void build(Request const &, std::vector<ServerCnf> const &, struct sockaddr_in const);
		std::string toString();
};

std::string	specRes(size_t);
std::string	mimeType(std::string);
std::string statusMessage(size_t);

std::string utostr(size_t n);
std::string timeToStr(time_t clock);
char		**getCGIArgs(char*, char*, char*);
