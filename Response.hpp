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
	private:
		typedef void (Response::*func)(Request const&, Location const&);
		typedef std::map<std::string, func> func_map;
		func_map	req_funcs;

	private:
		static std::string	_httpVersion;
		int					_statusCode;
		std::string			_statusMsg;
		Headers				_headers;
		std::string			_body;

		size_t	_getValidServerCnf(Request const &, std::vector<ServerCnf> const &,
				struct sockaddr_in const);
		size_t	_getValidLocation(Request const &, Locations const &);
		
		void _handleGetRequest(Request const &req, Location const &loc);
		void _handlePostRequest(Request const &req, Location const &loc);
		void _handleDeleteRequest(Request const &req, Location const &loc);

	public:
		Response();
		Response(Response const &);
		Response &operator= (Response const &);
		~Response();

		void build(Request const &, std::vector<ServerCnf> const &, struct sockaddr_in const);
		std::string toString();
};
