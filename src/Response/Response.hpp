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
# include <climits>
# include <csignal>
# include <cerrno>
// # include <strings.h>
# include <fcntl.h>
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
	std::string						upload_path;
	std::string						cgi_path;
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
	typedef bool (Response::*func)(size_t, std::string);
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
		// Response is ready
		bool				_ready;
		bool				_done;
		// upload
		int					_req_fd;
		std::string			_boundary;

	public:
		Response();
		Response(Response const &);
		Response &operator= (Response const &);
		~Response();

		bool		build(Request const &, std::vector<ServerCnf> const &, struct sockaddr_in const);
		bool		build(size_t=0);
		std::string get();
		bool		done();

	private:
		std::string	_readResBody(std::string &);
		bool		_isChunked();

		bool	_handleGetRequest(size_t, std::string);
		bool	_handlePostRequest(size_t, std::string);
		bool	_handleDeleteRequest(size_t, std::string);

		bool	_handleRegFile(std::string, struct stat);

		bool	_preHandleDir(std::string&, size_t);
		bool	_handleDir(std::string, struct stat, size_t);
		void	_internalRedir(std::string &);
		bool	_dirListing();
		bool	_readDir();
	
		bool	_handlePostDir(std::string, struct stat, size_t);
		bool	_handleUpload();
		bool	_parseBody();
		bool	_newPart(std::string &, Headers &);
		void	_moveUploadedFile(Headers &);

		bool	_handleCGI(std::string);
		char	**_getCGIArgs(std::string const &);
		char	**_getCGIEnv(std::string const &);
		bool	_waitProc();
		bool	_readFromCGI();
		bool	_getCgiHeaders(std::string &);
		bool	_isCGI(std::string const &);

		bool	_resRedir(size_t, size_t, std::string);
		
		bool	_resGenerate(size_t, std::string="");
		bool	_resGenerate(size_t, size_t);
		bool	_resGenerate(size_t, std::string, struct stat);

		size_t	_getValidServerCnf(std::vector<ServerCnf> const &, struct sockaddr_in const);
		size_t	_getValidLocation(Locations const &);
		bool	_checkLoc();

		int		_select(int);
};

std::string	specRes(size_t);
std::string	mimeType(std::string);
std::string statusMessage(size_t);

std::string utostr(size_t);
size_t		strtou(std::string);
std::string	strtoupper(std::string);
std::string timeToStr(time_t, bool=false);
char		**vectorToPtr(std::vector<std::string> &);
Headers		strToHeaders(char *);

std::string	getHyperlinkTag(std::string&, struct stat&);

void		freePtr(char **);

#endif
