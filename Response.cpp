#include "Response.hpp"

std::string Response::_httpVersion = "HTTP/1.1";

Response::Response() : _statusCode(0), _statusMsg(), _headers(), _body()
{
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
		return _res_generate(400, "Bad Request");
	}
	//std::cout << "server: " << srv.host << ":" << srv.port << std::endl;
	Location loc = srv.locs[_getValidLocation(req, srv.locs)];

	vs::iterator first = loc.accepted_methods.begin();
	vs::iterator last = loc.accepted_methods.end();
	if (loc.accepted_methods.size() && std::find(first, last, req.method) == last)
		return _res_generate(405, "Method Not Allowed");
	(this->*(_req_func(req.method)))(req, loc, srv.port);
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

Response::func Response::_req_func(std::string method)
{
	static std::map<std::string, func>	rf;

	if (rf.size()) return rf[method];

	rf["GET"] = &Response::_handleGetRequest;
	rf["POST"] = &Response::_handlePostRequest;
	rf["DELETE"] = &Response::_handleDeleteRequest;

	return rf[method];
}

// Request Methods Handling
void Response::_handleGetRequest(Request const &req, Location const &loc, size_t port)
{
	struct stat st;
	int			fd, st_ret;

	std::string fpath = loc.root + req.path;
	std::cout << "fpath: " << fpath << std::endl;
	if (loc.path == ".php") // Handle CGI
	{
		// if cgi_path is provided: send to cgi
		// else handle it as a static file
		std::cout << fpath << " to CGI" << std::endl;
		return ;
	}
	st_ret = stat(fpath.c_str(), &st);
	if (!st_ret && S_ISREG(st.st_mode)) // process regular file
	{
		std::cout << "static file" << std::endl;
		fd = open(fpath.c_str(), O_RDONLY);
		if (fd == -1)
		{
			return _res_generate(403, "Forbidden");
		}
		return _res_generate(200, "OK", fd, st);
	}
	if (!st_ret && S_ISDIR(st.st_mode)) // process directory
	{
		if (fpath[fpath.size() - 1] != '/') // request path does't end with '/'
		{
			return _res_generate(301, "Moves Permanently", req, port);
		}
		fpath += loc.index.size() ? loc.index : "index.html";
		std::cout << "static file index" << std::endl;
		// process regular file
		fd = open(fpath.c_str(), O_RDONLY);
		if (fd == -1)
		{
			if (loc.autoindex) return ; // list files
			return _res_generate(403, "Forbidden");
		}
		st_ret = fstat(fd, &st);
		return _res_generate(200, "OK", fd, st);
	}
	if (errno == EACCES)
		return _res_generate(403, "Forbidden");
	if (errno == ENOENT || errno == ENOTDIR)
		return _res_generate(404, "Not Found");
	std::cout << "Error " << errno << std::endl;
}

void Response::_handlePostRequest(Request const &req, Location const &loc, size_t port)
{
	(void)req;
	(void)loc;
	(void)port;
}

void Response::_handleDeleteRequest(Request const &req, Location const &loc, size_t port)
{
	(void)req;
	(void)loc;
	(void)port;
}

// Response Generation
void Response::_res_generate(size_t code, std::string msg)
{
	_statusCode = code;
	_statusMsg = msg;
	_headers["Content-type"] = "text/html";
	_headers["Content-Length"] = _statusMsg.size();
	_body = _spec_res(code);
}

void Response::_res_generate(size_t code, std::string msg, Request req, size_t port)
{
	_res_generate(code, msg);
	std::string lh = req.headers.find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + ft_utos(port) + req.path + "/";
}

void Response::_res_generate(size_t code, std::string msg, int fd, struct stat st)
{
	_statusCode = code;
	_statusMsg = msg;
	_headers["Content-Length"] = ft_utos(st.st_size);
	char buf[1024];
	int ret = st.st_size;
	while (ret)
	{
		bzero(buf, 1024);
		ret -= read(fd, buf, 1024);
		_body.append(buf);
	}
}

// Choosing Server Block and Location to use
size_t Response::_getValidServerCnf(Request const &req, std::vector<ServerCnf> const &serv_cnfs,
									struct sockaddr_in const addr)
{
	std::map<size_t, std::vector<std::string> > valid_cnf;
	std::map<size_t, std::vector<std::string> >::iterator it;
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

// Non-Members

std::string ft_utos(size_t n)
{
	std::stringstream	ss;
	std::string			str;

	ss << n;
	ss >> str;

	return str;
}