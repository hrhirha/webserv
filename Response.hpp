#ifndef RESPONSE_HPP
# define RESPONSE_HPP
# include <iostream>
# include <map>
# include <vector>
# include <algorithm>
# include <sstream>
# include <fstream>
# include <limits>
# include <cstdio>
# include <cstring>
# include <strings.h>
# include <fcntl.h>
# include <errno.h>
# include <unistd.h>
# include <dirent.h>
# include <sys/stat.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/select.h>
# include <arpa/inet.h>

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
	typedef bool (Response::*func)(size_t);
	func		_req_func(std::string);
	
	private:
		static std::string	_httpVersion;
		int					_statusCode;
		std::string			_statusMsg;
		Headers				_headers;
		std::string			_body;
		Request				_req;
		ServerCnf			_srv;
		Location			_loc;
		// for cgi
		int					_pid;
		int					_fd;
		time_t				_timeout;
		// for directory listing
		std::fstream		_fs;
		DIR					*_dir;
		std::string			_dpath;

	public:
		Response();
		Response(Response const &);
		Response &operator= (Response const &);
		~Response();

		bool build(Request const &, std::vector<ServerCnf> const &, struct sockaddr_in const);
		bool build();
		std::string toString();

	private:
		bool	_handleGetRequest(size_t);
		bool	_handlePostRequest(size_t);
		bool	_handleDeleteRequest(size_t);

		bool	_handleRegFile(std::string, struct stat);

		bool	_handleDir(std::string, struct stat, size_t);
		void	_internalRedir(std::string &);
		bool	_dirListing();
		bool	_readDir();

		bool	_handleCGI(std::string, std::string);
		bool	_waitProc();
		bool	_readFromCGI();
		bool	_getCgiHeaders(std::string &);
		bool	_isCGI(std::string const &);

		bool	_resRedir(size_t, size_t, std::string);
		
		bool	_resGenerate(size_t);
		bool	_resGenerate(size_t, size_t);
		bool	_resGenerate(size_t, std::string, struct stat);

		size_t	_getValidServerCnf(std::vector<ServerCnf> const &, struct sockaddr_in const);
		size_t	_getValidLocation(Locations const &);
		bool	_checkLoc();
};

std::string	specRes(size_t);
std::string	mimeType(std::string);
std::string statusMessage(size_t);

std::string utostr(size_t);
size_t		strtou(std::string);
std::string timeToStr(time_t, bool=false);
std::string	getHyperlinkTag(std::string&, struct stat&);

char		**getCGIArgs(char*, char*, char*);

#endif