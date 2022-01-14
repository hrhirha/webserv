#include "Response.hpp"

std::string Response::_httpVersion = "HTTP/1.1";

Response::Response() :
	_statusCode(0), _statusMsg(), _headers(), _body(),
	_req(), _srv(), _loc(),
	_pid(-1), _fd(-1), _timeout(0),
	_fs(), _dir(NULL), _dpath()
{
	_headers["Server"] = "WebServ/1.0";
	_headers["Connection"] = "close";
}

Response::Response(Response const &res) :
	_statusCode(res._statusCode), _statusMsg(res._statusMsg), _headers(res._headers),
	_body(res._body),
	_req(res._req), _srv(res._srv), _loc(res._loc),
	_pid(res._pid), _fd(res._fd), _timeout(res._timeout),
	_fs(), _dir(res._dir), _dpath(res._dpath)
{}

Response &Response::operator= (Response const &res)
{
	_statusCode = res._statusCode;
	_statusMsg = res._statusMsg;
	_headers = res._headers;
	_body = res._body;

	_req = res._req;
	_srv = res._srv;
	_loc = res._loc;

	_pid = res._pid;
	_fd = res._fd;

	_timeout = res._timeout;
	_dir = res._dir;
	_dpath = res._dpath;

	return *this;
}

Response::~Response()
{
	_statusMsg.clear();
	_headers.clear();
	_body.clear();
	// _req.~Request();
	// _srv.~ServerCnf();
}

bool Response::build(Request const &req, std::vector<ServerCnf> const &serv_cnfs,
	struct sockaddr_in const addr)
{

	if (_pid >= 0) return _waitProc();
	if (_dir) return _readDir();
	_req = req;
	_srv = serv_cnfs[_getValidServerCnf(serv_cnfs, addr)];
	_loc = _srv.locs[_getValidLocation(_srv.locs)];
	return build();
}

bool Response::build()
{
	if (_statusCode == 400)
		return _resGenerate(400);
	if (_checkLoc()) return true;
	return (this->*(_req_func(_req.method)))(_srv.port);
}

std::string Response::toString()
{
	if (_statusCode == 444) return "";
	std::string ret = Response::_httpVersion;

	ret.append(" " + utostr(_statusCode) + " " + _statusMsg + "\r\n");
	for (Headers::iterator it = _headers.begin(); it != _headers.end(); it++)
		ret.append(it->first + ": " + it->second + "\r\n");
	ret.append("\r\n");

	char buf[2048];
	while (1)
	{
		bzero(buf, 2048);
		if (read(_fd, buf, 2047) <= 0)
			break ;
		ret.append(buf);
	}
	close(_fd);
	return ret;
}

// Request Methods Handling
bool Response::_handleGetRequest(size_t port)
{
	struct stat st;
	int			st_ret;

	std::string fpath = _loc.root + _req.path;
	if (_isCGI(_loc.path)) // Handle CGI
		return _handleCGI(fpath, _req.query);
	st_ret = stat(fpath.c_str(), &st);
	if (!st_ret && S_ISREG(st.st_mode)) // Handle regular file
		return _handleRegFile(fpath, st);
	if (!st_ret /*&& S_ISDIR(st.st_mode)*/) // Handle directory
		return _handleDir(fpath, st, port);
	if (errno == ENOENT || errno == ENOTDIR)
		return _resGenerate(404);
	return _resGenerate(403); // if (errno == EACCES)
}

bool Response::_handlePostRequest(size_t port)
{
	(void)port;
	std::cout << "Post Request" << std::endl;
	return true;
}

bool Response::_handleDeleteRequest(size_t port)
{
	(void)port;
	std::cout << "Delete Request" << std::endl;
	return true;
}

// Static file
bool Response::_handleRegFile(std::string fpath, struct stat st)
{
	std::cout << "static file" << std::endl;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
	{
		return _resGenerate(403);
	}
	return _resGenerate(200, fpath, st);
}

// Directory
bool Response::_handleDir(std::string fpath, struct stat st, size_t port)
{
	std::cout << "directory" << std::endl;
	if (fpath[fpath.size() - 1] != '/') // _request path does't end with '/'
	{
		return _resGenerate(301, port);
	}
	_internalRedir(fpath); // if there is more appropraite location
	if (_checkLoc()) return true;
	if (_isCGI(_loc.path))
	{
		return _handleCGI(fpath, _req.query);
	}
	// process regular file
	bzero(&st, sizeof(struct stat));
	if (!stat(fpath.c_str(), &st) && S_ISDIR(st.st_mode) && _loc.autoindex)
		return _dirListing();
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
	{
		if (_loc.autoindex && errno == ENOENT) return _dirListing();
		return _resGenerate(403);
	}
	return _resGenerate(200, fpath, st);
}

void Response::_internalRedir(std::string &fpath)
{
	std::string new_rpath;
	
	size_t idx = _loc.index.find_first_not_of("/");
	std::cout << "idx = " << _loc.index << "\n";
	new_rpath = _req.path + _loc.index.substr(idx!=std::string::npos?idx:0);//(_loc.index.size() ? _loc.index : "index.html");
	std::cout << new_rpath << "\n";
	std::swap(_req.path, new_rpath);
	Location nloc = _srv.locs[_getValidLocation(_srv.locs)];
	// std::swap(_req.path, new_rpath);
	fpath += _loc.index.size() ? _loc.index : "index.html";
	if (_loc.path != nloc.path)
		fpath = nloc.root + _req.path + (_req.path[_req.path.size()-1] == '/' ? (nloc.index.size() ? nloc.index : "index.html") : "");
	_loc = nloc;
	_dpath = fpath.substr(0, fpath.find_last_of("/")+1);
}

bool Response::_dirListing()
{
	struct timeval	tv;

	std::cout << "dir listing" << std::endl;
	gettimeofday(&tv, NULL);
	_body = "/tmp/dirlist_" + utostr(tv.tv_sec*1e6 + tv.tv_usec) + ".html";
	_fs.open(_body.c_str(), std::ios_base::out);
	_fs << "<html>\n\
<head><title>Index of "+_req.path+"</title></head>\n\
<body>\n\
<h1>Index of "+_req.path+"</h1><hr><pre><a href=\"../\">../</a>\n";
	if (!(_dir = opendir(_dpath.c_str())))
	{
		return _resGenerate(500);
	}
	return _readDir();
}

bool Response::_readDir()
{
	struct stat		st;
	struct dirent	*ent;
	struct timeval	tv;
	time_t			utm;

	gettimeofday(&tv, NULL);
	utm = tv.tv_usec;
	while ((ent = readdir(_dir)))
	{
		gettimeofday(&tv, NULL);
		if (tv.tv_usec - utm >= 1e4)
			return false;
		std::string name = ent->d_name;
		if (name[0] == '.') continue ;
		stat((_dpath+name).c_str(), &st);
		_fs << getHyperlinkTag(name, st);
	}
	_fs << "</pre><hr></body>\n</html>";
	closedir(_dir);
	_fs.close();
	_fd = open(_body.c_str(), O_RDONLY);
	_statusCode = 200;
	_statusMsg = statusMessage(200);
	_headers["Date"] = timeToStr(time(NULL));
	_headers["Content-Type"] = "text/html";
	_headers["Transfer-Encoding"] = "chunked";
	return true;
}

// CGI
bool Response::_handleCGI(std::string fpath, std::string query)
{
	struct timeval	tv;
	char			**args;

	std::cout << "CGI handling" << std::endl;
	char cgi_path[] = "/usr/bin/php-cgi";
	// char cgi_path = _loc.cgi.c_str();
	// char cgi_path[] = "/Users/hrhirha/goinfre/.brew/bin/php-cgi";
	args = getCGIArgs(cgi_path, (char*)fpath.c_str(), (char*)query.c_str());

	gettimeofday(&tv, NULL);
	_body = "/tmp/cgi_" + utostr(tv.tv_sec*1e6 + tv.tv_usec) + ".res";
	_fd = open(_body.c_str(), O_RDWR | O_CREAT, 0644);
	if ((_pid = fork()) == -1) return _resGenerate(500);
	gettimeofday(&tv, NULL);
	_timeout = tv.tv_sec;
	if(!_pid)
	{
		dup2(_fd, 1);
		execve(args[0], args, NULL);
		_exit(1);
	}
	delete [] args;
	close(_fd);
	_fd = -1;
	return _waitProc();
}

bool Response::_waitProc()
{
	if (_pid > 0)
	{
		struct timeval	tv;
		int				status = 0;
		usleep(1e4);
		int result = waitpid(_pid, &status, WNOHANG);
		status = WEXITSTATUS(status);
		if (result == -1 || (status > 0 && status < 255)) // Error
		{
			remove(_body.c_str());
			_body.clear();
			return _resGenerate(500);
		}
		if (result == 0) // child still alive
		{
			gettimeofday(&tv, NULL);
			if (tv.tv_sec - _timeout > 60) // timeout
			{
				kill(_pid, SIGTERM);
				wait(NULL);
				_pid = 0;
				remove(_body.c_str());
				_body.clear();
				return _resGenerate(504);
			}
			return false;
		}
		_pid = 0;
	}
	// child has exited
	return _readFromCGI();
}

bool Response::_readFromCGI()
{
	struct timeval		tv;
	fd_set				set;
	char				buf[10001];
	static std::string	buffer;

	tv.tv_sec = 0; tv.tv_usec = 1e3;
	_fd = (_fd == -1) ? open(_body.c_str(), O_RDONLY) : _fd;
	FD_ZERO(&set);
	FD_SET(_fd, &set);
	if (select(_fd+1, &set, NULL, NULL, &tv) == -1)
	{
		close(_fd);
		remove(_body.c_str());
		_body.clear();
		FD_ZERO(&set);
		return _resGenerate(500);
	}
	std::cout << FD_ISSET(_fd, &set) << std::endl;
	if (!FD_ISSET(_fd, &set))
		return false;
	bzero(buf, 10001);
	read(_fd, buf, 10000);
	FD_ZERO(&set);
	buffer.append(buf);
	if (_getCgiHeaders(buffer))
	{
		lseek(_fd, buffer.size()*-1, SEEK_CUR);
		Headers::iterator it = _headers.find("Status");
		if (it != _headers.end())
		{
			_statusCode = atoi(it->second.c_str());
			_statusMsg = statusMessage(_statusCode);
			_headers.erase(it);
			return true;
		}
		_statusCode = 200;
		_statusMsg = statusMessage(200);
		return true;
	}
	return false;
}

bool Response::_getCgiHeaders(std::string &buffer)
{
	static bool done = false;
	std::string h("");
	size_t i, j;

	if (done) return true;

	i = buffer.find("\r\n\r\n");
	if (i == std::string::npos)
		return false;
	done = true;
	h = buffer.substr(0, i+2);
	buffer = buffer.substr(i+4);
	i = h.find("\r\n");
	while (i != std::string::npos)
	{
		std::string sub = h.substr(0, i);
		h = h.substr(i+2);
		j = sub.find(":");
		if (j != std::string::npos)
		{
			std::string ct = sub.substr(0, j);
			ct = (ct == "Content-type") ? "Content-Type" : ct;
			_headers[ct] = sub.substr(j+2);
		}
		i = h.find("\r\n");
	}
	_headers["Date"] = timeToStr(time(NULL));
	_headers["Transfer-Encoding"] = "chunked";
	return true;
}

bool Response::_isCGI(std::string const &path)
{
	size_t dot = path.find_last_of('.');
	std::string ext = dot != std::string::npos ? path.substr(dot) : "";
	if (ext == ".php")
		return true;
	return false;
}

// Redirection
bool Response::_resRedir(size_t code, size_t port, std::string redir)
{
	_resGenerate(code);
	if (redir[0] != '/')
	{
		_headers["Location"] = redir;
		return true;
	}
	std::string lh = _req.headers.find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + utostr(port) + redir;
	return true;
}

// Response Generation
bool Response::_resGenerate(size_t code)
{
	struct timeval	tv;
	std::fstream	fs;

	_statusCode = code;
	_statusMsg = statusMessage(code);
	// check if code is in error_page
	gettimeofday(&tv, NULL);
	_body = "/tmp/" + utostr(code) + "_" + utostr(tv.tv_sec*1e6 + tv.tv_usec) + ".html";
	fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
	std::string sr = specRes(code);
	fs << sr;
	fs.close();
	_headers["Content-Type"] = "text/html";
	_headers["Content-Length"] = utostr(sr.size());
	_headers["Date"] = timeToStr(time(NULL));
	_fd = open(_body.c_str(), O_RDONLY);
	return true;
}

bool Response::_resGenerate(size_t code, size_t port)
{
	std::string lh = _req.headers.find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + utostr(port) + _req.path + "/";
	return _resGenerate(code);
}

bool Response::_resGenerate(size_t code, std::string fpath, struct stat st)
{
	std::stringstream ss;

	_statusCode = code;
	_statusMsg = statusMessage(code);
	_headers["Content-Length"] = utostr(st.st_size);
	_headers["Date"] = timeToStr(time(NULL));
	_headers["Content-Type"] = mimeType(fpath);
	_headers["Last-Modified"] = timeToStr(st.st_mtime);
	ss << "\"" << std::hex << st.st_mtime << "-" << std::hex << st.st_size << "\"";
	ss >> _headers["ETag"];
	_body = fpath;
	return true;
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
		std::string fname = _req.path.substr(j+1);
		size_t dot = fname.find_last_of(".");
		std::string rext = (dot != std::string::npos) ? fname.substr(dot) : "";
		std::string lpath = locs[i].path;
		if ((rext == lpath) || _req.path == lpath)
			return i;
		if (path.size() >= locs[i].path.size() && locs[i].path == path.substr(0, locs[i].path.size())
			&& (loc_idx == -1 || locs[i].path.size() > locs[loc_idx].path.size()))
		{
			loc_idx = i;
		}
	}
	return loc_idx;
}

bool Response::_checkLoc()
{
	vs::iterator first = _loc.accepted_methods.begin();
	vs::iterator last = _loc.accepted_methods.end();
	if (_loc.accepted_methods.size() && std::find(first, last, _req.method) == last)
		return _resGenerate(405);
	if (_loc.redirect.first)
		return _resRedir(_loc.redirect.first, _srv.port, _loc.redirect.second);
	return false;
}
