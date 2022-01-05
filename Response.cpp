#include "Response.hpp"

std::string Response::_httpVersion = "HTTP/1.1";

Response::Response() :
	_statusCode(0), _statusMsg(), _headers(), _body(), _req(), _srv()
{
	_headers["Server"] = "WebServ/1.0";
	_headers["Connection"] = "close";
}

Response::Response(Response const &res) :
	_statusCode(res._statusCode), _statusMsg(res._statusMsg),
	_headers(res._headers), _body(res._body), _req(res._req), _srv(res._srv)
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
	// remove(_body.c_str());
}

void Response::build(Request const &req, std::vector<ServerCnf> const &serv_cnfs,
	struct sockaddr_in const addr)
{
	_req = req;
	_srv = serv_cnfs[_getValidServerCnf(serv_cnfs, addr)];
	if (_statusCode == 400)
		return _resGenerate(400);
	Location loc = _srv.locs[_getValidLocation(_srv.locs)];

	vs::iterator first = loc.accepted_methods.begin();
	vs::iterator last = loc.accepted_methods.end();
	if (loc.accepted_methods.size() && std::find(first, last, _req.method) == last)
		return _resGenerate(405);
	if (loc.redirect.first)
		return _resRedir(loc.redirect.first, _srv.port, loc.redirect.second);
	(this->*(_req_func(_req.method)))(loc, _srv.port);
}

std::string Response::toString()
{
	std::string ret = Response::_httpVersion;
	int			fd;

	ret.append(" " + utostr(_statusCode) + " " + _statusMsg + "\r\n");
	for (Headers::iterator it = _headers.begin(); it != _headers.end(); it++)
		ret.append(it->first + ": " + it->second + "\r\n");
	ret.append("\r\n");
	fd = open(_body.c_str(), O_RDONLY);
	char buf[2048];
	long size = atol(_headers["Content-Length"].c_str());
	while (size > 0)
	{
		bzero(buf, 2048);
		size -= read(fd, buf, 2047);
		ret.append(buf);
	}
	close(fd);
	return ret;
}

// Request Methods Handling
void Response::_handleGetRequest(Location const &loc, size_t port)
{
	struct stat st;
	int			st_ret;

	std::string fpath = loc.root + _req.path;
	std::cout << "fpath: " << fpath << std::endl;
	if (loc.path == ".php") // Handle CGI
		return _handleCGI(loc, fpath, _req.query);
	st_ret = stat(fpath.c_str(), &st);
	if (!st_ret && S_ISREG(st.st_mode)) // Handle regular file
		return _handleRegFile(fpath, st);
	if (!st_ret /*&& S_ISDIR(st.st_mode)*/) // Handle directory
		return _handleDir(fpath, st, loc, port);
	if (errno == ENOENT || errno == ENOTDIR)
		return _resGenerate(404);
	return _resGenerate(403); // if (errno == EACCES)
	// std::cout << "Error " << errno << std::endl;
}

void Response::_handlePostRequest(Location const &loc, size_t port)
{
	(void)loc;
	(void)port;
	std::cout << "Post Request" << std::endl;
}

void Response::_handleDeleteRequest(Location const &loc, size_t port)
{
	(void)loc;
	(void)port;
	std::cout << "Delete Request" << std::endl;
}

// Request path handling
void Response::_handleRegFile(std::string fpath, struct stat st)
{
	std::cout << "static file" << std::endl;
	int fd = open(fpath.c_str(), O_RDONLY);
	if (fd == -1)
	{
		return _resGenerate(403);
	}
	_resGenerate(200, fd, fpath, st);
	close(fd);
}

void Response::_handleDir(std::string fpath, struct stat st,
	Location const &loc, size_t port)
{
	if (fpath[fpath.size() - 1] != '/') // _request path does't end with '/'
	{
		return _resGenerate(301, port);
	}
	fpath += loc.index.size() ? loc.index : "index.html";
	_req.path = fpath;
	Location cgi_loc = _srv.locs[_getValidLocation(_srv.locs)];
	if (fpath.substr(fpath.size()-4) == ".php" && cgi_loc.path == ".php")
	{
		return _handleCGI(cgi_loc, fpath, _req.query);
	}
	std::cout << "static file index" << std::endl;
	// process regular file
	int fd = open(fpath.c_str(), O_RDONLY);
	if (fd == -1)
	{
		if (loc.autoindex) return ; // list files
		return _resGenerate(403);
	}
	fstat(fd, &st);
	_resGenerate(200, fd, fpath, st);
	close(fd);
}

void Response::_handleCGI(Location const &loc, std::string fpath, std::string query)
{
	int		pid;
	int		fd[2];
	char	**args;

	(void)loc;
	std::cout << "CGI handling" << std::endl;
	int fdes = open(fpath.c_str(), O_RDONLY);
	if ( fdes == -1 && (errno == ENOENT || errno == ENOTDIR))
		return _resGenerate(404);
	if (fdes == -1) return _resGenerate(403); // if (errno == EACCES)
	close(fdes);
	char cgi_path[] = "/usr/bin/php-cgi";
	args = getCGIArgs(cgi_path, (char*)fpath.c_str(), (char*)query.c_str());
	if (pipe(fd) == -1) return _resGenerate(500);
	if ((pid = fork()) == -1) return _resGenerate(500);
	if(!pid)
	{
		fcntl(fd[1], F_SETFL, O_NONBLOCK);
		dup2(fd[1], STDOUT_FILENO);
		execve(args[0], args, NULL);
	}
	wait(NULL);
	close(fd[1]);
	delete [] args;
	return _getCGIRes(fd[0]);
	close(fd[0]);
}

void Response::_getCGIRes(int fd)
{
	fd_set			set;
	struct timeval	tv;
	std::fstream	fs;
	size_t			size = 0;

	gettimeofday(&tv, NULL);
	std::string file = "/tmp/cgi_" + utostr(tv.tv_sec*1e6 + tv.tv_usec) + ".txt";
	fs.open(file.c_str(), std::ios_base::out | std::ios_base::binary);

	FD_ZERO(&set);
	FD_SET(fd, &set);
	while (1)
	{
		int rd =_readFromCGI(fd, fs, &set, size);
		if (rd == -1)
		{
			fs.close();
			close(fd);
			remove(file.c_str());
			_headers.erase("Content-type");
			return _resGenerate(500);
		}
		if (!rd) break ;
	}
	if (size)
	{
		_body = file;
		_headers["Content-Length"] = utostr(size);
	}
	_headers["Date"] = timeToStr(time(NULL));
	fs.close();
	close(fd);
	_body = file;
	_statusCode = 200;
	_statusMsg = statusMessage(200);
}

int Response::_readFromCGI(int fd, std::fstream &fs, fd_set *set, size_t &size)
{
	char			buf[8];
	(void)set;
	// struct timeval	tv;
	// fd_set			rset;
	
	// tv.tv_sec = 0; tv.tv_usec = 1e3;
	// FD_ZERO(&rset);
	// rset = *set;
	// if (select(fd+1, &rset, NULL, NULL, &tv) == -1)
	// {
	// 	close(fd);
	// 	return -1;
	// }
	// std::cout << FD_ISSET(fd, &rset) << std::endl;
	// if (!FD_ISSET(fd, &rset))
	// 	return 0;
	bzero(buf, 8);
	if (!read(fd, buf, 7)) return 0;
		// std::cout << "DEBUG" << std::endl;
	_body.append(buf);
	if (_getCgiHeaders())
	{
		Headers::iterator it = _headers.find("Status");
		if (it != _headers.end())
		{
			// std::cout << it->second << std::endl;
			_headers.erase(it);
			return -1;
		}
		size += _body.size();
		fs << _body;
		_body.clear();
	}
	// FD_CLR(fd, set);
	return 1;
}

bool Response::_getCgiHeaders()
{
	static bool done = false;
	std::string h("");
	size_t i, j;

	if (done) return true;

	i = _body.find("\r\n\r\n");
	if (i == std::string::npos)
		return false;
	done = true;
	h = _body.substr(0, i+2);
	_body = _body.substr(i+4);
	i = h.find("\r\n");
	while (i != std::string::npos)
	{
		std::string sub = h.substr(0, i);
		h = h.substr(i+2);
		j = sub.find(":");
		if (j != std::string::npos)
		{
			_headers[sub.substr(0, j)] = sub.substr(j+2);
		}
		i = h.find("\r\n");
	}
	return true;
}

// Redirection
void Response::_resRedir(size_t code, size_t port, std::string redir)
{
	_resGenerate(code);
	if (redir[0] != '/')
	{
		_headers["Location"] = redir;
		return ;
	}
	std::string lh = _req.headers.find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + utostr(port) + redir;
}

// Response Generation
void Response::_resGenerate(size_t code)
{
	struct timeval	tv;
	std::fstream	fs;

	gettimeofday(&tv, NULL);
	_body = "/tmp/" + utostr(code) + "_" + utostr(tv.tv_sec*1e6 + tv.tv_usec) + ".html";
	fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
	_statusCode = code;
	_statusMsg = statusMessage(code);
	// check if code is in error_page
	fs << specRes(code);
	fs.close();
	_headers["Content-Type"] = "text/html";
	_headers["Content-Length"] = utostr(_body.size());
	_headers["Date"] = timeToStr(time(NULL));
}

void Response::_resGenerate(size_t code, size_t port)
{
	_resGenerate(code);
	std::string lh = _req.headers.find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + utostr(port) + _req.path + "/";
}

void Response::_resGenerate(size_t code, int fd, std::string fpath, struct stat st)
{
	(void)fd;
	_statusCode = code;
	_statusMsg = statusMessage(code);
	_headers["Content-Length"] = utostr(st.st_size);
	_headers["Date"] = timeToStr(time(NULL));
	_headers["Content-Type"] = mimeType(fpath);
	_headers["Last-Modified"] = timeToStr(st.st_mtime);
	_body = fpath;
	// char buf[1024];
	// int ret = st.st_size;
	// while (ret)
	// {
	// 	bzero(buf, 1024);
	// 	ret -= read(fd, buf, 1024);
	// 	_body.append(buf);
	// }
}

// Choosing Server Block and Location to use
size_t Response::_getValidServerCnf(std::vector<ServerCnf> const &serv_cnfs,
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
	h_it = _req.headers.find("Host");
	if (h_it == _req.headers.end() || h_it->second.find(' ') != std::string::npos)
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

size_t Response::_getValidLocation(Locations const &locs)
{
	size_t j;
	int loc_idx = -1;
	if (_req.path[0] != '/') // _request path doesn't start with '/'
	{
		_statusCode = 400;
		return 0;
	}
	j = _req.path.find_last_of('/');
	std::string path = _req.path.substr(0, j + 1);
	for (size_t i = 0; i < locs.size(); i++)
	{
		if (_req.path.size() > 4 && _req.path.substr(_req.path.size() - 4) == ".php" && locs[i].path == ".php")
			return i;
		if (path.size() >= locs[i].path.size() && locs[i].path == path.substr(0, locs[i].path.size()) && (loc_idx == -1 || locs[i].path.size() > locs[loc_idx].path.size()))
		{
			loc_idx = i;
		}
	}
	return loc_idx;
}
