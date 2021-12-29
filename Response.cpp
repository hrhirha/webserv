#include "Response.hpp"

std::string Response::_httpVersion = "HTTP/1.1";

Response::Response() : req_funcs(), _statusCode(0), _statusMsg(), _headers(), _body()
{
	req_funcs["GET"] = &Response::_handleGetRequest;
	req_funcs["POST"] = &Response::_handlePostRequest;
	req_funcs["DELETE"] = &Response::_handleDeleteRequest;
	
	_headers["Server"] = "WebServ/1.0";
}

Response::Response(Response const &res) :
	_statusCode(res._statusCode), _statusMsg(res._statusMsg),
	_headers(res._headers), _body(res._body)
{}

Response &Response::operator= (Response const &res)
{
	_statusCode = res._statusCode;
	_statusMsg = res._statusMsg;
	_headers = res._headers;
	_body = res._body;
	return *this;
}

Response::~Response()
{
	_headers.clear();
}

void Response::build(Request const &req, std::vector<ServerCnf> const &serv_cnfs,
		struct sockaddr_in const addr)
{
	ServerCnf srv = serv_cnfs[_getValidServerCnf(req, serv_cnfs, addr)];
	if (_statusCode == 400)
	{
		_statusMsg = "Bad Request";
		return ;
	}
	//std::cout << "server: " << srv.host << ":" << srv.port << std::endl;
	Location loc = srv.locs[_getValidLocation(req, srv.locs)];

	(this->*(req_funcs.find(req.method)->second))(req, loc);
}

std::string ft_utos(size_t n)
{
	std::stringstream	ss;
	std::string			str;

	ss << n;
	ss >> str;

	return str;
}

std::string Response::toString()
{
	std::string ret = Response::_httpVersion;

	ret.append(" " + ft_utos(_statusCode) + " " + _statusMsg + "\r\n");
	for (Headers::iterator it = _headers.begin(); it != _headers.end(); it++)
		ret.append(it->first + ": " + it->second + "\r\n");
	ret.append("\r\n");
	ret.append(_body);

	return ret;
}

void Response::_handleGetRequest(Request const &req, Location const &loc)
{

	std::string fpath = loc.root + req.path;
	std::cout << "fpath: " << fpath << std::endl;
	if (loc.path == ".php")
	{
		// if cgi_path is provided: send to cgi
		// else handle it as a static file
		std::cout << fpath << " to CGI" << std::endl;
		// Handle CGI
		return ;
	}
	struct stat st;
	int st_ret = stat(fpath.c_str(), &st);
	if (!st_ret && S_ISREG(st.st_mode))
	{
		std::cout << "static file" << std::endl;
		// process regular file
		int fd = open(fpath.c_str(), O_RDONLY);
		if (fd == -1 && errno == EACCES)
		{
			_statusCode = 403;
			_statusMsg = "Forbidden";
			return ;
		}
		_statusCode = 200;
		_statusMsg = "OK";
		_headers["Content-Length"] = ft_utos(st.st_size);
		char buf[1024];
		while (read(fd, buf, 1024))
			_body.append(buf);
	}
	else if (!st_ret && S_ISDIR(st.st_mode))
	{
		// process directory
		if (fpath[fpath.size() - 2] != '/')
		{
			_statusCode = 301;
			_statusMsg = "Moves Permanently";
			// return 301 moved Permanently
			// Location: http://req.headers["Host"]:srv.port
		}
		else if (loc.index.size())
		{
			fpath += loc.index;
		}
	}
	else
	{
		if (errno == EACCES)
		{
			_statusCode = 403;
			_statusMsg = "Forbidden";
		}
		else if (errno == ENOENT || errno == ENOTDIR)
		{
			_statusCode = 404;
			_statusMsg = "Not Found";
		}
		std::cout << "Error " << errno << std::endl;
	}
}

void Response::_handlePostRequest(Request const &req, Location const &loc)
{
	(void)req;
	(void)loc;
}

void Response::_handleDeleteRequest(Request const &req, Location const &loc)
{
	(void)req;
	(void)loc;
}

size_t Response::_getValidServerCnf(Request const &req, std::vector<ServerCnf> const &serv_cnfs,
									struct sockaddr_in const addr)
{
	std::map<size_t, std::vector<std::string>> valid_cnf;
	std::map<size_t, std::vector<std::string>>::iterator it;
	std::map<std::string, std::string>::const_iterator h_it;

	// find which server block to use based on addr.host, addr.port

	for (size_t i = 0; i < serv_cnfs.size(); i++)
	{
		if (addr.sin_addr.s_addr == inet_addr(serv_cnfs[i].host.c_str()) && addr.sin_port == htons(serv_cnfs[i].port))
			valid_cnf.insert(std::make_pair(i, serv_cnfs[i].server_names));
	}
	if (valid_cnf.size() == 1)
		return valid_cnf.begin()->first;

	// if multiple servers are listening on the same host:port
	// check the "Host" header against server blocks' "server_names"

	std::string host = "";
	h_it = req.headers.find("Host");
	if (h_it == req.headers.end() || h_it->second.find(' ') != std::string::npos)
	{
		// 400 Bad Request
		_statusCode = 400;
		return 0;
	}
	host = h_it->second;
	size_t idx = host.find(":");
	host = host.substr(0, idx != std::string::npos ? idx : host.size());
	for (it = valid_cnf.begin(); it != valid_cnf.end(); it++)
	{
		if (std::find(it->second.begin(), it->second.end(), host) != it->second.end())
			return it->first;
	}
	return 0;
}

size_t Response::_getValidLocation(Request const &req, Locations const &locs)
{
	size_t j;
	int loc_idx = -1;
	if (req.path[0] != '/') // request path doesn't start with '/'
	{
		_statusCode = 400;
		return 0;
	}
	j = req.path.find_last_of('/');
	std::string path = req.path.substr(0, j + 1);
	for (size_t i = 0; i < locs.size(); i++)
	{
		if (req.path.size() > 4 && req.path.substr(req.path.size() - 4) == ".php" && locs[i].path == ".php")
			return i;
		if (path.size() >= locs[i].path.size() && locs[i].path == path.substr(0, locs[i].path.size()) && (loc_idx == -1 || locs[i].path.size() > locs[loc_idx].path.size()))
		{
			loc_idx = i;
		}
	}
	return loc_idx;
}
