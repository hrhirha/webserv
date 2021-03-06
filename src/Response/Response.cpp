#include "Response.hpp"

std::string Response::_httpVersion = "HTTP/1.1";

Response::Response() : _statusCode(0), _statusMsg(), _headers(), _body(),
					   _req(), _srv(), _loc(),
					   _pid(-1), _fd(-1), _timeout(0), _cgi_headers(),
					   _fs(), _dir(NULL), _dpath(),
					   _ready(false), _done(false),
					   _req_fd(-1), _boundary(), _first_call(true)
{
	_headers["Server"] = "WebServ/1.0";
	_headers["Connection"] = "close";
	_headers["Content-Type"] = "application/octet-stream";
}

Response::Response(Response const &res) : _statusCode(0), _statusMsg(), _headers(), _body(),
										  _req(), _srv(), _loc(),
										  _pid(-1), _fd(-1), _timeout(0), _cgi_headers(),
										  _fs(), _dir(NULL), _dpath(),
										  _ready(false), _done(false),
										  _req_fd(-1), _boundary()
{
	*this = res;
}

Response &Response::operator=(Response const &res)
{
	_statusCode = res._statusCode;
	_statusMsg = res._statusMsg;
	_headers = res._headers;
	_body = res._body;

	_req = res._req;
	_srv = res._srv;
	_loc = res._loc;

	_pid = res._pid;
	if (res._fd > -1)
		_fd = dup(res._fd);
	_timeout = res._timeout;
	_cgi_headers = res._cgi_headers;

	if (res._fs.is_open())
		_fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
	_dir = res._dir;
	_dpath = res._dpath;

	_ready = res._ready;
	_done = res._done;

	if (res._req_fd > -1)
		_req_fd = dup(res._req_fd);
	_req_fd = res._req_fd;
	_boundary = res._boundary;

	_first_call = res._first_call;

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

	_pid = -1;
	if (_fd > -1)
	{
		close(_fd);
		_fd = -1;
	}
	_timeout = 0;
	_cgi_headers.clear();

	_fs.close();
	if (_dir)
	{
		closedir(_dir);
		_dir = NULL;
	}
	_dpath.clear();

	if (_req_fd > -1)
	{
		close(_req_fd);
		_req_fd = -1;
	}
	_boundary.clear();
}

bool Response::build(Request const &req)
{
	if (_ready)
		return true;
	if (_pid >= 0)
		return _waitProc();
	if (_dir)
		return _readDir();
	if (_req_fd > 0)
		return _parseBody();
	_req = req;
	Headers::iterator it = _req.getheaders().find("Connection");
	if (it != _req.getheaders().end() && it->second == "keep-alive")
		_headers["Connection"] = "keep-alive";
	_statusCode = _req.getError();
	_srv = _req.getServerBlock();
	_loc = _getValidLocation(_srv.getlocs());
	if (_statusCode != 0) // chaeck if there was an error while parsing
		return _resGenerate(_statusCode);
	if (_checkLoc())
		return true;

	std::string fpath = _loc.getLocation_root() + _req.getpath();
	return (this->*(_req_func(_req.getmethod())))(_srv.getPort(), fpath);
}

std::string Response::get()
{
	std::string ret("");

	if (!_first_call)
		return _readResBody(ret);

	if (_statusCode == 444)
	{
		_done = true;
		_first_call = true;
		close(_fd);
		_fd = -1;
		return "";
	}
	ret = Response::_httpVersion;
	ret.append(" " + utostr(_statusCode) + " " + _statusMsg + "\r\n");
	for (Headers::iterator it = _headers.begin(); it != _headers.end(); it++)
		ret.append(it->first + ": " + it->second + "\r\n");
	// append cgi headers
	for (size_t i = 0; i < _cgi_headers.size(); i++)
		ret.append(_cgi_headers[i].first + ": " + _cgi_headers[i].second + "\r\n");
	ret.append("\r\n");
	_first_call = false;
	return _readResBody(ret);
}

bool Response::done()
{
	return _done;
}

std::string Response::_readResBody(std::string &ret)
{
	char buf[1048576];
	std::stringstream ss;

	int sel = _select(_fd);
	if (sel <= 0) // select failed | _fd is not in fd_set
		return "";
	bzero(buf, 1048576);
	errno = 0;
	int rd = read(_fd, buf, 1048576);
	if (rd == -1) return "";
	if (rd == 0)
	{
		_done = true;
		_first_call = true;
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
	int st_ret;

	if (!_loc.getPathCgi().empty())
		return _handleCGI(fpath);
	st_ret = stat(fpath.c_str(), &st);
	if (!st_ret && S_ISREG(st.st_mode)) // Handle regular file
		return _handleRegFile(fpath, st);
	if (!st_ret) // Handle directory
		return _handleDir(fpath, st, port);
	if (errno == ENOENT || errno == ENOTDIR)
		return _resGenerate(404);
	return _resGenerate(403); // if (errno == EACCES)
}

bool Response::_handlePostRequest(size_t port, std::string fpath)
{
	struct stat st;
	int st_ret;

	errno = 0;
	st_ret = stat(fpath.c_str(), &st);
	if (st_ret == -1)
		return _resGenerate(errno == ENOENT || errno == ENOTDIR ? 404 : 403);
	if (!_loc.getPathCgi().empty())
		return _handleCGI(fpath);
	if (!st_ret && S_ISDIR(st.st_mode)) // Handle directory
		return _handlePostDir(fpath, st, port);
	return _resGenerate(405);
}

bool Response::_handleDeleteRequest(size_t port, std::string fpath)
{
	(void)port;

	if (!_loc.getPathCgi().empty())
		return _handleCGI(fpath);
	return _resGenerate(405);
}

// get Regular file
bool Response::_handleRegFile(std::string fpath, struct stat st)
{
	errno = 0;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
		return _resGenerate(errno == EMFILE ? 500 : 403);
	return _resGenerate(200, fpath, st);
}

// Directory GET/POST/DELETE
bool Response::_preHandleDir(std::string &fpath, size_t port, bool &cgi)
{
	if (fpath[fpath.size() - 1] != '/')
		return _resGenerate(301, port);

	std::string tmpreq = _req.getpath();
	while (1)
	{
		if (_checkLoc())
		{
			_req.getpath() = tmpreq;
			return true;
		}
		if (!_loc.getPathCgi().empty())
		{
			_req.getpath() = tmpreq;
			return (cgi = true);
		}
		std::string tmp = _loc.getPathOfLocation();
		_internalRedir(fpath); // if there is more appropraite location
		if (tmp == _loc.getPathOfLocation())
			break ;
	}
	_req.getpath() = tmpreq;
	return false;
}

// get Directory
bool Response::_handleDir(std::string fpath, struct stat st, size_t port)
{
	bool cgi = false; 
	
	if (_preHandleDir(fpath, port, cgi))
	{
		if (cgi) return _handleCGI(fpath);
		return true;
	}

	// process regular file
	bzero(&st, sizeof(struct stat));
	if (!stat(fpath.c_str(), &st) && S_ISDIR(st.st_mode) && _loc.getAutoIndex())
		return _dirListing();
	errno = 0;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
	{
		if (_loc.getAutoIndex() && errno == ENOENT)
			return _dirListing();
		return _resGenerate(errno == EMFILE ? 500 : 403);
	}
	return _resGenerate(200, fpath, st);
}

void Response::_internalRedir(std::string &fpath)
{
	std::string new_rpath;

	size_t idx = _loc.getIndex().find_first_not_of("/");
	new_rpath = _req.getpath() + ((_req.getpath()[_req.getpath().size()-1] != '/') ? "" : _loc.getIndex().substr(idx != std::string::npos ? idx : 0));
	_req.getpath() = new_rpath;
	Location nloc = _getValidLocation(_srv.getlocs());
	fpath += (fpath[fpath.size()-1] != '/') ? "" : _loc.getIndex().size() ? _loc.getIndex() : "index.html";
	if (_loc.getPathOfLocation() != nloc.getPathOfLocation() && nloc.getPathCgi().empty())
	{
		fpath = nloc.getLocation_root() + _req.getpath() + (_req.getpath()[_req.getpath().size() - 1] == '/' ? (nloc.getIndex().size() ? nloc.getIndex() : "index.html") : "");
	}
	_loc = nloc;
	_dpath = fpath.substr(0, fpath.find_last_of("/") + 1);
}

bool Response::_dirListing()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	_body = "/tmp/dirlist_" + utostr(tv.tv_sec * 1e6 + tv.tv_usec) + ".html";
	_fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
	_fs << "<html>\n\
<head><title>Index of " +
			   _req.getpath() + "</title></head>\n\
<body>\n\
<h1>Index of " +
			   _req.getpath() + "</h1><hr><pre><a href=\"../\">../</a>\n";
	if (!(_dir = opendir(_dpath.c_str())))
	{
		return _resGenerate(500);
	}
	return _readDir();
}

bool Response::_readDir()
{
	struct stat st;
	struct dirent *ent;
	struct timeval tv;
	time_t utm;

	gettimeofday(&tv, NULL);
	utm = tv.tv_usec;
	while ((ent = readdir(_dir)))
	{
		std::string name = ent->d_name;
		if (name[0] == '.')
			continue;
		stat((_dpath + name).c_str(), &st);
		_fs << getHyperlinkTag(name, st);
		gettimeofday(&tv, NULL);
		if (tv.tv_usec - utm >= 1e3)
			return false;
	}
	_fs << "</pre><hr></body>\n</html>";
	closedir(_dir);
	_dir = NULL;
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
	bool cgi = false;
	if (_preHandleDir(fpath, port, cgi))
	{
		if (cgi) return _handleCGI(fpath);
		return true;
	}
	if (!_loc.getUpload_path().empty())
		return _handleUpload();

	bzero(&st, sizeof(struct stat));
	if (!stat(fpath.c_str(), &st) && S_ISDIR(st.st_mode))
		return _resGenerate(403);
	errno = 0;
	_fd = open(fpath.c_str(), O_RDONLY);
	if (_fd == -1)
		return _resGenerate(errno == EMFILE ? 500 : 403);
	close(_fd);
	_fd = -1;
	return _resGenerate(405);
}

bool Response::_handleUpload()
{
	struct timeval	tv;
	std::string		multipart = "multipart/form-data";
	std::string		ct = _req.getheaders()["Content-Type"];

	gettimeofday(&tv, NULL);
	_body = "/tmp/upload_" + utostr(tv.tv_sec * 1e6 + tv.tv_usec);
	_fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
	if (ct.substr(0, multipart.size()) == multipart)
	{
		size_t i = ct.find("boundary=");
		_boundary = i != std::string::npos ? ct.substr(i + 9) : "";
		_req_fd = open(_req.getbody().c_str(), O_RDONLY);
		if (_req_fd == -1)
		{
			_fs.close();
			remove(_body.c_str());
			return _resGenerate(errno == EMFILE ? 500 : 200);
		}
		return _parseBody();
	}
	std::string cmd = "cp " + _req.getbody() + " " + _loc.getUpload_path()
		+ (_loc.getUpload_path()[_loc.getUpload_path().size() - 1] != '/' ? "/" : "")
		+ _body.substr(_body.find_last_of('/'));
	_fs.close();
	system(cmd.c_str());
	remove(_body.c_str());
	return _resGenerate(200);
}

bool Response::_parseBody()
{
	char buf[1048576];
	static std::string buffer;
	static Headers th;

	int sel = _select(_req_fd);
	if (sel == -1) // select failed
	{
		_fs.close();
		close(_req_fd);
		_req_fd = -1;
		_ready = true;
		return _resGenerate(500);
	}
	if (sel == 0)
		return false; // fd not in fd_set
	int rd = read(_req_fd, buf, 1048576);
	buffer.append(buf, rd);
	std::string line = "--" + _boundary + "\r\n";
	if (buffer.size() >= line.size() && buffer.substr(0, line.size()) == line)
	{
		size_t end = buffer.find("\r\n\r\n");
		if (end == std::string::npos)
			return false;
		std::string header_str = buffer.substr(line.size(), end + 2);
		buffer = buffer.substr(end + 4);
		th = strToHeaders((char *)header_str.c_str());
	}
	return _newPart(buffer, th, rd);
}

bool Response::_newPart(std::string &buffer, Headers &th, int rd)
{
	std::string line = "--" + _boundary + "\r\n";
	std::string end_line = "--" + _boundary + "--\r\n";
	size_t new_part = buffer.find(line);
	size_t last_part = buffer.find(end_line);
	if (new_part == std::string::npos && last_part == std::string::npos)
	{
		if (buffer.size() > end_line.size())
		{
			_fs << buffer.substr(0, buffer.size() - end_line.size());
			buffer = buffer.substr(buffer.size() - end_line.size());
		}
		if (rd == 0)
		{
			_fs << buffer;
			buffer.clear();
			_moveUploadedFile(th, false);
			close(_req_fd);
			_req_fd = -1;
			return _resGenerate(200);
		}
		return false;
	}
	if (new_part != std::string::npos)
	{
		_fs << buffer.substr(0, new_part - 2);
		buffer = buffer.substr(new_part);
		_moveUploadedFile(th);
		return false;
	}
	_fs << buffer.substr(0, last_part - 2);
	buffer.clear();
	_moveUploadedFile(th);
	close(_req_fd);
	_req_fd = -1;
	return _resGenerate(200);
}

void Response::_moveUploadedFile(Headers &th, bool multi)
{
	_fs.close();
	std::string filename = th["Content-Disposition"];
	size_t is_file = filename.find("filename");
	if (is_file != std::string::npos)
	{
		filename = filename.substr(is_file + 10);
		filename = filename.substr(0, filename.size() - 1);
		if (!filename.empty())
		{
			std::string cmd = "cp " + _body + " " + _loc.getUpload_path()
				+ (_loc.getUpload_path()[_loc.getUpload_path().size() - 1] != '/' ? "/" : "") + filename;
			system(cmd.c_str());
		}
	}
	if (!multi)
	{
		std::string cmd = "cp " + _body + " " + _loc.getUpload_path()
			+ (_loc.getUpload_path()[_loc.getUpload_path().size() - 1] != '/' ? "/" : "");
		system(cmd.c_str());

	}
	remove(_body.c_str());
	_body.clear();
	th.clear();
}

// CGI
bool Response::_handleCGI(std::string fpath)
{
	struct timeval tv;
	char **args;
	char **env;

	args = _getCGIArgs(fpath);
	env = _getCGIEnv(fpath);

	gettimeofday(&tv, NULL);
	_body = "/tmp/cgi_" + utostr(tv.tv_sec * 1e6 + tv.tv_usec) + ".html";
	_fd = open(_body.c_str(), O_RDWR | O_CREAT, 0644);
	if ((_pid = fork()) == -1)
		return _resGenerate(500);

	gettimeofday(&tv, NULL);
	_timeout = tv.tv_sec;
	if (!_pid)
	{
		if (!_req.getbody().empty())
		{
			int fd = open(_req.getbody().c_str(), O_RDONLY);
			if (fd == -1)
				_exit(1);
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

	args.push_back(std::string(_loc.getPathCgi()));
	args.push_back(fpath);
	return vectorToPtr(args);
}

char **Response::_getCGIEnv(std::string const &fpath)
{
	std::vector<std::string> v;
	std::string arg;

	Headers::iterator f = _req.getheaders().begin();
	Headers::iterator l = _req.getheaders().end();
	for (; f != l; f++)
	{
		v.push_back("HTTP_" + strtoupper(f->first) + "=" + f->second);
	}
	v.push_back("REDIRECT_STATUS=200");
	v.push_back("SERVER_NAME=" + _srv.getserver_names()[0]);
	v.push_back("SERVER_PORT=" + utostr(_srv.getPort()));
	v.push_back("SERVER_ADDR=" + _srv.getHost());
	v.push_back("REMOTE_PORT=");
	v.push_back("REMOTE_ADDR=");
	v.push_back("SERVER_SOFTWARE=webserv/1.0");
	v.push_back("GATEWAY_INTERFACE=CGI/1.1");
	v.push_back("REQUEST_SCHEME=http");
	v.push_back("SERVER_PROTOCOL=HTTP/1.1");
	v.push_back("DOCUMENT_ROOT=" + _loc.getLocation_root());
	v.push_back("DOCUMENT_URI=" + fpath.substr(_loc.getLocation_root().size()));
	v.push_back("REQUEST_URI=" + _req.getpath() + (_req.getquery().empty() ? "" : ("?" + _req.getquery())));
	v.push_back("SCRIPT_NAME=" + fpath.substr(_loc.getLocation_root().size()));
	v.push_back("CONTENT_LENGTH=" + _req.getheaders()["Content-Length"]);
	v.push_back("CONTENT_TYPE=" + _req.getheaders()["Content-Type"]);
	v.push_back("REQUEST_METHOD=" + _req.getmethod());
	v.push_back("QUERY_STRING=" + _req.getquery());
	v.push_back("SCRIPT_FILENAME=" + fpath);
	v.push_back("FCGI_ROLE=RESPONDER");

	return vectorToPtr(v);
}

bool Response::_waitProc()
{
	if (_pid > 0)
	{
		struct timeval tv;
		int status = 0;
		//usleep(1e4);
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
	char buf[1048576];
	static std::string buffer("");

	_fd = (_fd == -1) ? open(_body.c_str(), O_RDONLY) : _fd;
	int sel = _select(_fd);
	if (sel == -1)
	{
		remove(_body.c_str());
		_body.clear();
		return _resGenerate(500);
	}
	if (sel == 0)
		return false;
	int rd = read(_fd, buf, 1048576);
	buffer.append(buf, rd);
	if (_getCgiHeaders(buffer) || rd <= 0)
	{
		_ready = true;
		_headers["Date"] = timeToStr(time(NULL));
		if (!_headers.count("Content-Length"))
			_headers["Transfer-Encoding"] = "chunked";
		lseek(_fd, buffer.size() * -1, SEEK_CUR);
		Headers::iterator it = _headers.find("Status");
		if (it != _headers.end())
		{
			_statusCode = atoi(it->second.c_str());
			_statusMsg = statusMessage(_statusCode);
			_headers.erase(it);
			buffer.clear();
			return true;
		}
		_statusCode = 200;
		_statusMsg = statusMessage(200);
		buffer.clear();
		return true;
	}
	return false;
}

bool Response::_getCgiHeaders(std::string &buffer)
{
	std::string h("");
	size_t i, j;

	i = buffer.find("\r\n\r\n");
	if (i == std::string::npos)
		return false;
	h = buffer.substr(0, i + 2);
	buffer = buffer.substr(i + 4);
	i = h.find("\r\n");
	while (i != std::string::npos)
	{
		std::string sub = h.substr(0, i);
		h = h.substr(i + 2);
		j = sub.find(":");
		if (j != std::string::npos)
		{
			std::string key = sub.substr(0, j);
			key = (key == "Content-type") ? "Content-Type" : key;
			std::string val = sub.substr(j + 2);
			// cgi header
			if (_headers.count(key) && key != "Content-Type")
			{
				std::pair<std::string,std::string> new_header = std::make_pair(key, val);
				_cgi_headers.push_back(new_header);
			}
			else
				_headers[key] = val;
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
	if (code < 301 || (code > 303 && code != 307 && code != 308))
	{
		_resGenerate(code, redir);
		return true;
	}
	_resGenerate(code);
	if (redir[0] != '/')
	{
		_headers["Location"] = redir;
		return true;
	}
	std::string lh = _req.getheaders().find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + utostr(port) + redir;
	return true;
}

// Response Generation
bool Response::_resGenerate(size_t code, std::string redir)
{
	struct timeval tv;
	std::fstream fs;
	std::string sr("");

	_statusCode = code;
	_statusMsg = statusMessage(code);
	// check if code is in error_page
	error error_pages = _srv.geterror_pages();
	_body = error_pages.second;
	struct stat st;
	int st_ret = stat(_body.c_str(), &st);
	if (!st_ret) _headers["Content-Length"] = utostr(st.st_size);
	if (std::find(error_pages.first.begin(), error_pages.first.end(), _statusCode) == error_pages.first.end() || st_ret == -1)
	{
		gettimeofday(&tv, NULL);
		_body = "/tmp/" + utostr(code) + "_" + utostr(tv.tv_sec * 1e6 + tv.tv_usec) + ".html";
		fs.open(_body.c_str(), std::ios_base::out | std::ios_base::binary);
		sr = redir.empty() ? specRes(code) : redir;
		fs << sr;
		fs.close();
		_headers["Content-Length"] = utostr(sr.size());
	}
	if (redir.empty())
		_headers["Content-Type"] = "text/html";
	_headers["Date"] = timeToStr(time(NULL));
	_fd = open(_body.c_str(), O_RDONLY);
	_ready = true;
	return true;
}

bool Response::_resGenerate(size_t code, size_t port)
{
	std::string lh = _req.getheaders().find("Host")->second;
	size_t i = lh.find(':');
	lh = (i == std::string::npos) ? lh : lh.substr(0, i);
	_headers["Location"] = "http://" + lh + ":" + utostr(port) + _req.getpath() + "/";
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

// Choosing Location to use

Location Response::_getValidLocation(Locations const &locs)
{
	size_t j;
	int loc_idx = -1;

	j = _req.getpath().find_last_of('/');
	std::string path = _req.getpath().substr(0, j + 1);
	std::string fname = _req.getpath().substr(j + 1);
	size_t dot = fname.find_last_of(".");
	std::string rext = (dot != std::string::npos) ? fname.substr(dot) : "";
	for (size_t i = 0; i < locs.size(); i++)
	{
		std::string lpath = locs[i].getPathOfLocation();
		if ((rext == lpath) || _req.getpath() == lpath)
			return locs[i];
		lpath += lpath[lpath.size()-1] == '/' ? "" : "/";
		std::string loc_p = locs[i].getPathOfLocation();
		if (path.size() >= loc_p.size() && loc_p == path.substr(0, loc_p.size())
			&& (loc_idx == -1 || loc_p.size() > locs[loc_idx].getPathOfLocation().size()))
		{
			loc_idx = i;
		}
	}
	if (loc_idx < 0)
		return Location().getDefLoc();
	return locs[loc_idx];
}

bool Response::_checkLoc()
{
	Methods methods = _loc.getAcceptedMethods();
	vs::iterator first = methods.begin();
	vs::iterator last = methods.end();
	if (methods.size() && std::find(first, last, _req.getmethod()) == last)
		return _resGenerate(403);
	if (_loc.getRedirect().first)
		return _resRedir(_loc.getRedirect().first, _srv.getPort(), _loc.getRedirect().second);
	return false;
}

int Response::_select(int fd)
{
	struct timeval tv;
	fd_set set;

	tv.tv_sec = 0;
	tv.tv_usec = 1e3;
	FD_ZERO(&set);
	FD_SET(fd, &set);
	if (select(fd + 1, &set, NULL, NULL, &tv) == -1)
	{
		close(fd);
		FD_ZERO(&set);
		return -1;
	}
	if (!FD_ISSET(fd, &set))
		return 0;
	return 1;
}
