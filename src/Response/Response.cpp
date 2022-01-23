#include "Response.hpp"

std::string Response::_httpVersion = "HTTP/1.1";

Response::Response() :
	_statusCode(0), _statusMsg(), _headers(), _body(),
	_req(), _srv(), _loc(),
	_pid(-1), _fd(-1), _timeout(0),
	_fs(), _dir(NULL), _dpath(),
	_ready(false), _done(false),
	_req_fd(-1), _boundary()
{
	_headers["Server"] = "WebServ/1.0";
	_headers["Connection"] = "close";
	_headers["Content-Type"] = "application/octet-stream";
}

Response::Response(Response const &res) :
	_statusCode(res._statusCode), _statusMsg(res._statusMsg), _headers(res._headers),
	_body(res._body),
	_req(res._req), _srv(res._srv), _loc(res._loc),
	_pid(res._pid), _fd(res._fd), _timeout(res._timeout),
	_fs(), _dir(res._dir), _dpath(res._dpath),
	_ready(res._ready), _done(res._done),
	_req_fd(res._req_fd), _boundary(res._boundary)
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

	_ready = res._ready;
	_done = res._done;

	_req_fd = res._req_fd;
	_boundary = res._boundary;

	return *this;
}

Response::~Response()
{
	_statusMsg.clear();
	_headers.clear();
	_body.clear();
	// _req.~Request();
	// _srv.~ServerCnf();
	// _loc.~Location();
}

bool Response::build(Request const &req, std::vector<ServerCnf> const &serv_cnfs,
	struct sockaddr_in const addr)
{
	if (_ready) return true;
	if (_pid >= 0) return _waitProc();
	if (_dir) return _readDir();
	if (_req_fd > 0) return _parseBody();
	_req = req;
	_srv = serv_cnfs[_getValidServerCnf(serv_cnfs, addr)];
	_loc = _srv.locs[_getValidLocation(_srv.locs)];
	return build(_statusCode);
}

bool Response::build(size_t code)
{
	if (code != 0)
		return _resGenerate(code);
	if (_checkLoc()) return true;

	std::string fpath = _loc.root + _req.path;
	return (this->*(_req_func(_req.method)))(_srv.port, fpath);
}

std::string Response::get()
{
	std::string	ret("");
	static bool	first_call = true;

	if (!first_call)
		return _readResBody(ret);

	if (_statusCode == 444)
	{
		_done = true;
		close(_fd);
		return "";
	}
	ret = Response::_httpVersion;
	ret.append(" " + utostr(_statusCode) + " " + _statusMsg + "\r\n");
	for (Headers::iterator it = _headers.begin(); it != _headers.end(); it++)
		ret.append(it->first + ": " + it->second + "\r\n");
	ret.append("\r\n");
	first_call = false;
	return _readResBody(ret);
}

bool Response::done()
{
	return _done;
}

std::string Response::_readResBody(std::string &ret)
{
	char				buf[1048576];
	std::stringstream	ss;

	int sel = _select(_fd);
	if (sel <= 0) // select failed | _fd is not in fd_set
		return "";
	int rd = read(_fd, buf, 1048576);
	if (rd <= 0)
	{
		_done = true;
		close(_fd);
	}
	if (_isChunked())
	{
		std::string b(buf, rd);
		ss << std::hex << b.size();
		ret.append(ss.str() + "\r\n" + b + "\r\n");
		return ret;
	}
	ret.append(buf, rd);
	return ret;
}

bool Response::_isChunked()
{
	Headers::iterator it = _headers.find("Transfer-Encoding");
	if (it != _headers.end() && it->second == "chunked")
		return true;
	return false;
}

// Request Methods Handling
bool Response::_handleGetRequest(size_t port, std::string fpath)
{
	struct stat st;
	int			st_ret;

	st_ret = stat(fpath.c_str(), &st);
	if (!st_ret && S_ISREG(st.st_mode)) // Handle regular file
		return _handleRegFile(fpath, st);
	if (!st_ret /*&& S_ISDIR(st.st_mode)*/) // Handle directory
		return _handleDir(fpath, st, port);
	if (errno == ENOENT || errno == ENOTDIR)
		return _resGenerate(404);
	return _resGenerate(403); // if (errno == EACCES)
}

bool Response::_handlePostRequest(size_t port, std::string fpath)
{
	struct stat st;
	int			st_ret;

	st_ret = stat(fpath.c_str(), &st);
	if (_isCGI(_loc.path)) // Handle CGI
		return _handleCGI(fpath);
	if (!st_ret && S_ISDIR(st.st_mode)) // Handle directory
		return _handlePostDir(fpath, st, port);
	return _resGenerate(405);
}

bool Response::_handleDeleteRequest(size_t port, std::string fpath)
{
	struct stat st;
	int			st_ret;

	(void)port;
	(void)fpath;
	st_ret = stat(fpath.c_str(), &st);
	if (_isCGI(_loc.path))
		return _handleCGI(fpath);
	return _resGenerate(405);
}

// get Regular file
bool Response::_handleRegFile(std::string fpath, struct stat st)
{
	if (_isCGI(_loc.path)) // Handle CGI
		return _handleCGI(fpath);
	errno = 0;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
		return _resGenerate(errno == EMFILE ? 500 : 403);
	return _resGenerate(200, fpath, st);
}

// Directory GET/POST/DELETE
bool Response::_preHandleDir(std::string &fpath, size_t port)
{
	if (fpath[fpath.size() - 1] != '/' /*&& fpath.substr(fpath.find_last_of("/")) != std::string("..")*/) // _request path does't end with '/'
		return _resGenerate(301, port);

	// chdir(fpath.c_str());
	// char *cwd = get_current_dir_name();
	// fpath = cwd;
	// if (fpath.find(_loc.root) == std::string::npos)
	// 	_req.path = "/";
	// fpath = _loc.root + _req.path;
	// free(cwd);

	_internalRedir(fpath); // if there is more appropraite location
	if (_checkLoc())
		return true;
	return false;
}

// get Directory
bool Response::_handleDir(std::string fpath, struct stat st, size_t port)
{
	if (_preHandleDir(fpath, port))
		return true;
	if (_isCGI(_loc.path))
		return _handleCGI(fpath);
	// process regular file
	bzero(&st, sizeof(struct stat));
	if (!stat(fpath.c_str(), &st) && S_ISDIR(st.st_mode) && _loc.autoindex)
		return _dirListing();
	errno = 0;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
	{
		if (_loc.autoindex && errno == ENOENT) return _dirListing();
		return _resGenerate(errno == EMFILE ? 500 : 403);
	}
	return _resGenerate(200, fpath, st);
}

void Response::_internalRedir(std::string &fpath)
{
	std::string new_rpath;
	
	size_t idx = _loc.index.find_first_not_of("/");
	new_rpath = _req.path + _loc.index.substr(idx!=std::string::npos ? idx : 0);//(_loc.index.size() ? _loc.index : "index.html");
	std::string tmp = _req.path;
	_req.path = new_rpath;
	Location nloc = _srv.locs[_getValidLocation(_srv.locs)];
	std::cout << "loc: " << nloc.path << "\n";
	fpath += _loc.index.size() ? _loc.index : "index.html";
	if (_loc.path != nloc.path)
		fpath = nloc.root + _req.path + (_req.path[_req.path.size()-1] == '/' ? (nloc.index.size() ? nloc.index : "index.html") : "");
	_loc = nloc;
	_dpath = fpath.substr(0, fpath.find_last_of("/")+1);
	_req.path = tmp;
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
	_ready = true;
	return true;
}

// post Directory
bool Response::_handlePostDir(std::string fpath, struct stat st, size_t port)
{
	(void)st;
	if (_preHandleDir(fpath, port))
		return true;
	// _loc.upload_path = "/uploads/";
	if (!_loc.upload_path.empty())
		return _handleUpload();
	if (_isCGI(_loc.path))
		return _handleCGI(fpath);	
	bzero(&st, sizeof(struct stat));
	if (!stat(fpath.c_str(), &st) && S_ISDIR(st.st_mode))
		return _resGenerate(403);
	errno = 0;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
		return _resGenerate(errno == EMFILE ? 500 : 403);
	return _resGenerate(405);
}

bool Response::_handleUpload()
{
	std::string multipart = "multipart/form-data";
	std::string ct = _req.headers["Content-Type"];

	if (ct.substr(0, multipart.size()) == multipart)
	{
		size_t i = ct.find("boundary=");
		_boundary = i != std::string::npos ? ct.substr(i+9) : "";
		_req_fd = open(_req.body.c_str(), O_RDONLY);
		if (_req_fd == -1)
			return _resGenerate(errno == EMFILE ? 500 : 200);
		return _parseBody();
	}
	return _resGenerate(200);
}

bool Response::_parseBody()
{
	char				buf[1048576];
	static std::string	buffer;
	static Headers		th;
	struct timeval		tv;

	int sel = _select(_req_fd);
	if (sel == -1) // select failed
	{
		close(_req_fd);
		_req_fd = -1;
		_ready = true;
		return _resGenerate(500);
	}
	if (sel == 0) return false; // fd not in fd_set
	int rd = read(_req_fd, buf, 1048576);
	buffer.append(buf, rd);
	std::string line = "--"+_boundary+"\r\n";
	if (buffer.size() >= line.size() && buffer.substr(0, line.size()) == line)
	{
		size_t end = buffer.find("\r\n\r\n");
		if (end == std::string::npos) return false;
		std::string header_str = buffer.substr(line.size(), end+2);
		buffer = buffer.substr(end+4);
		th = strToHeaders((char*)header_str.c_str());
		gettimeofday(&tv, NULL);
		_body = "/tmp/upload_" + utostr(tv.tv_sec*1e6 + tv.tv_usec);
		_fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
	}
	return _newPart(buffer, th);
}

bool Response::_newPart(std::string &buffer, Headers &th)
{
	std::string line = "--"+_boundary+"\r\n";
	std::string end_line = "--"+_boundary+"--\r\n";
	size_t new_part = buffer.find(line);
	size_t last_part = buffer.find(end_line);
	if (new_part == std::string::npos && last_part == std::string::npos)
	{
		if (buffer.size() > end_line.size())
		{
			_fs << buffer.substr(0, buffer.size()-end_line.size());
			buffer = buffer.substr(buffer.size()-end_line.size());
		}
		return false;
	}
	if (new_part != std::string::npos)
	{
		_fs << buffer.substr(0, new_part-2);
		buffer = buffer.substr(new_part);
		_moveUploadedFile(th);
		return false;
	}
	_fs << buffer.substr(0, last_part-2);
	buffer.clear();
	_moveUploadedFile(th);
	close(_req_fd);
	_req_fd = -1;
	return _resGenerate(200);
}

void	Response::_moveUploadedFile(Headers &th)
{
	_fs.close();
	std::string filename = th["Content-Disposition"];
	size_t is_file = filename.find("filename");
	if (is_file != std::string::npos)
	{
		filename = filename.substr(is_file+10);
		filename = filename.substr(0, filename.size()-1);
		std::string cmd = "mv " + _body + " " + _loc.root + _loc.upload_path + "/" + filename;
		system(cmd.c_str());
	}
	remove(_body.c_str());
	_body.clear();
	th.clear();
}

// get CGI
bool Response::_handleCGI(std::string fpath)
{
	struct timeval	tv;
	char			**args;
	char			**env;

	_loc.cgi_path = "/usr/bin/php-cgi";
	args = _getCGIArgs(fpath);
	env = _getCGIEnv(fpath);

	gettimeofday(&tv, NULL);
	_body = "/tmp/cgi_" + utostr(tv.tv_sec*1e6 + tv.tv_usec) + ".html";
	_fd = open(_body.c_str(), O_RDWR | O_CREAT, 0644);
	if ((_pid = fork()) == -1) return _resGenerate(500);

	gettimeofday(&tv, NULL);
	_timeout = tv.tv_sec;
	if(!_pid)
	{
		if (!_req.body.empty())
		{
			int fd = open(_req.body.c_str(), O_RDONLY);
			if (fd == -1) _exit(1);
			dup2(fd, 0);
		}
		dup2(_fd, 1);
		execve(args[0], args, env);
		_exit(1);
	}
	freePtr(args);
	freePtr(env);
	close(_fd);
	_fd = -1;
	return _waitProc();
}

char **Response::_getCGIArgs(std::string const &fpath)
{
	(void)fpath;
	std::vector<std::string> args;

	args.push_back(std::string(_loc.cgi_path));
	return vectorToPtr(args);
}

char **Response::_getCGIEnv(std::string const &fpath)
{
	std::vector<std::string> v;
	std::string			arg;

	Headers::iterator f = _req.headers.begin();
	Headers::iterator l = _req.headers.end();
	for (; f != l; f++)
	{
		v.push_back("HTTP_"+strtoupper(f->first)+"="+f->second);
	}
	v.push_back("REDIRECT_STATUS=200");
	v.push_back("SERVER_NAME=" + _srv.server_names[0]);
	v.push_back("SERVER_PORT=" + utostr(_srv.port));
	v.push_back("SERVER_ADDR=" + _srv.host);
	v.push_back("REMOTE_PORT=");
	v.push_back("REMOTE_ADDR=");
	v.push_back("SERVER_SOFTWARE=webserv/1.0");
	v.push_back("GATEWAY_INTERFACE=CGI/1.1");
	v.push_back("REQUEST_SCHEME=http");
	v.push_back("SERVER_PROTOCOL=HTTP/1.1");
	v.push_back("DOCUMENT_ROOT=" + _loc.root);
	v.push_back("DOCUMENT_URI=" + fpath.substr(_loc.root.size()));
	v.push_back("REQUEST_URI=" + _req.path);
	v.push_back("SCRIPT_NAME=" + fpath.substr(_loc.root.size()));
	v.push_back("CONTENT_LENGTH=" + _req.headers["Content-Length"]);
	v.push_back("CONTENT_TYPE=" + _req.headers["Content-Type"]);
	v.push_back("REQUEST_METHOD=" + _req.method);
	v.push_back("QUERY_STRING=" + _req.query);
	v.push_back("SCRIPT_FILENAME=" + fpath);
	v.push_back("FCGI_ROLE=RESPONDER");

	return vectorToPtr(v);
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
	char				buf[1048576];
	static std::string	buffer;

	_fd = (_fd == -1) ? open(_body.c_str(), O_RDONLY) : _fd;
	int sel = _select(_fd);
	if (sel == -1)
	{
		remove(_body.c_str());
		_body.clear();
		return _resGenerate(500);
	}
	if (sel == 0) return false;
	int rd = read(_fd, buf, 1048576);
	buffer.append(buf, rd);
	if (_getCgiHeaders(buffer) || rd <= 0)
	{
		_ready = true;
		_headers["Date"] = timeToStr(time(NULL));
		_headers["Transfer-Encoding"] = "chunked";
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
	if (code < 301 || (code > 303 && code != 307 && code != 308))
		return true;
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
	_ready = true;
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
	_ready = true;
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
		return _resGenerate(403);
	if (_loc.redirect.first)
		return _resRedir(_loc.redirect.first, _srv.port, _loc.redirect.second);
	return false;
}

int	Response::_select(int fd)
{
	struct timeval		tv;
	fd_set				set;

	tv.tv_sec = 0; tv.tv_usec = 1e3;
	FD_ZERO(&set);
	FD_SET(fd, &set);
	if (select(fd+1, &set, NULL, NULL, &tv) == -1)
	{
		close(fd);
		FD_ZERO(&set);
		// _resGenerate(500);
		return -1;
	}
	if (!FD_ISSET(fd, &set))
		return 0;
	return 1;
}
