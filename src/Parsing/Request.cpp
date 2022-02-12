/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/21 09:37:15 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/02/11 18:06:43 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include <unistd.h>
// status code defines
#define BAD_REQUEST 400
#define HTTP_VERSION_NOT_SUPPORTED 505

// isdigit fonction
bool isStrDigit(const std::string &str)
{
	for (size_t i = 0; i < str.size(); i++)
		if (!std::isdigit(str[i]))
			return false;
	return true;
}
// trim fonction
std::string trim(const std::string &s)
{
	std::string::const_iterator start = s.begin();
	while (start != s.end() && std::isspace(*start))
	{
		start++;
	}
	std::string::const_iterator end = s.end();
	do
	{
		end--;
	} while (std::distance(start, end) > 0 && std::isspace(*end));

	return std::string(start, end + 1);
}

//	fonction for split
std::vector<std::string> split(std::string line, char delimiter)
{
	std::vector<std::string> strings;
	if (line.empty())
		return strings;
	std::istringstream f(line);
	std::string str;
	while (std::getline(f, str, delimiter))
	{
		if (!str.empty() && str != "\n")
			strings.push_back(str);
	}
	return strings;
}

// fonctions for conversion
size_t to_sizeT(std::string str)
{
	size_t sizeT;
	std::istringstream iss(str);
	iss >> sizeT;

	return sizeT;
}

// Function to convert hexadecimal to decimal
int convert(char num[])
{
	int len = strlen(num);
	int base = 1;
	int temp = 0;

	for (int i = len - 1; i >= 0; i--)
	{
		if (num[i] >= '0' && num[i] <= '9')
		{
			temp += (num[i] - 48) * base;
			base = base * 16;
		}
		else if (num[i] >= 'A' && num[i] <= 'F')
		{
			temp += (num[i] - 55) * base;
			base = base * 16;
		}
	}
	return temp;
}

Request::Request() {}

//default constructor
Request::Request(std::string &req, std::vector<ServerCnf> srvs, struct sockaddr_in addr) : _method(""), _path(""), _query(""),
																						   _version(""), _body(""), _RequestCompleted(false), _HeadersCompleted(false),
																						   _Error(0), _BodySize(0), _HowMuchShouldRead(0)
{
	// _ServerBlock = _getValidServerCnf(srvs, addr);
	Parse(req);
};

// copy constructor
Request::Request(Request const &copy)
{
	*this = copy;
};

// destructor
Request::~Request(void){};

// operator equal
Request &Request::operator=(Request const &copy)
{
	this->_method = copy._method;
	this->_path = copy._path;
	this->_query = copy._query;
	this->_version = copy._version;
	this->_body = copy._body;
	this->_headers = copy._headers;
	this->_RequestCompleted = copy._RequestCompleted;

	return *this;
};

// getters
std::string &Request::getmethod(void) { return this->_method; };
std::string &Request::getpath(void) { return this->_path; };
std::string &Request::getquery(void) { return this->_query; };
std::string &Request::getversion(void) { return this->_version; };
std::string &Request::getbody(void) { return this->_body; };
Headers &Request::getheaders(void) { return this->_headers; };
ServerCnf &Request::getServerBlock(void) { return this->_ServerBlock; };
bool Request::isRequestCompleted(void) { return this->_RequestCompleted; };

// print content method
void Request::print(void)
{
	Headers::iterator beg = _headers.begin();
	Headers::iterator end = _headers.end();

	std::cout << "Request is completed : [" << _RequestCompleted << "]." << std::endl;
	std::cout << "Body Size : [" << _BodySize << "]." << std::endl;
	std::cout << "error found = [" << _Error << "]" << std::endl;
	std::cout << "_method = [" << _method << "] | _path = [" << _path << "]  | query = [" << _query << "] | version = [" << _version << "] . " << std::endl;
	for (; beg != end; beg++)
		std::cout << "value = [" << beg->first << "]->[" << beg->second << "]." << std::endl;
}

// generate name method
std::string Request::generateNameFile(void)
{
	struct timeval time_now;
	std::stringstream ss;

	gettimeofday(&time_now, 0);
	time_t msecs_time = time_now.tv_usec + (time_now.tv_sec * 1e6);
	ss << "File[";
	ss << msecs_time;
	ss << "]";
	return ss.str();
}

// parse Methods
void Request::Parse(std::string &req)
{
	req = _buffer.append(req);
	size_t EndOfHeaders = req.find("\r\n\r\n");
	std::cout <<  "h hhh " << _Error << std::endl;
	if (!_HeadersCompleted && EndOfHeaders == std::string::npos)
		return;
	else if (!_HeadersCompleted && EndOfHeaders != std::string::npos)
	{
		parseHeaders(req, EndOfHeaders);
		req = _buffer;
		_buffer = "";
	}
	else if ((_HeadersCompleted && _method == "GET") || _Error)
	{
		_RequestCompleted = true;
		return;
	}
	// Body treatment
	FillBody(req);
	if (_RequestCompleted || _Error)
	{
		_fstream.close();
		if (_Error)
		{
			std::string concat = "rm " + _body;
			system(concat.c_str());
		}
		_RequestCompleted = true;
		print();
	}
};

// Chunked request Fonction
void Request::ChunkedRequest(std::string &req)
{
	std::string line;
	size_t found;
	size_t len;

	if (req.empty())
		return;
	do
	{
		if (_HowMuchShouldRead)
		{
			found = req.find("\r\n");
			line = (found != std::string::npos) ? req.substr(0, found) : req;
			len = line.length();
			_Error = (_HowMuchShouldRead < len) ? BAD_REQUEST : _Error;
			_BodySize += len;
			_HowMuchShouldRead -= len;
			_fstream << line;
			if (found == std::string::npos)
				return;
			req = req.substr(found + 2);
		}
		found = req.find("\r\n");
		_HowMuchShouldRead = convert(const_cast<char *>(req.substr(0, found).c_str()));
		if (!_HowMuchShouldRead)
		{
			_RequestCompleted = true;
			return;
		}
		req = req.substr(found + 2);
		found = req.find("\r\n");
		line = (found != std::string::npos) ? req.substr(0, found) : req;
		len = line.length();
		_BodySize += len;
		_Error = (_HowMuchShouldRead < len) ? BAD_REQUEST : _Error;
		_HowMuchShouldRead -= len;
		_fstream << line;
		req = (std::string::npos != found) ? req.substr(found + 2) : req;
	} while (!_Error && found != std::string::npos);
}

// fonction to fill the body
void Request::FillBody(std::string &req)
{
	if (_body.empty())
	{
		_body = generateNameFile();
		_fstream.open(_body.c_str());
		if (!_fstream.good())
			std::cout << "Error isn't good file stream" << std::endl;
	}
	if (_headers.find("Content-Length") != _headers.end())
	{
		_buffer = "";
		size_t sizeT = to_sizeT(_headers.find("Content-Length")->second);
		_BodySize += req.size();
		_fstream << req;
		_RequestCompleted = (sizeT == _BodySize) ? true : _RequestCompleted;
		_Error = (sizeT < _BodySize) ? BAD_REQUEST : _Error;
	}
	else if (_headers.find("Transfer-Encoding") != _headers.end() && _headers["Transfer-Encoding"] == "chunked")
		ChunkedRequest(req);
	else 
		_RequestCompleted = true;
}

// fonction to parse all headers
void Request::parseHeaders(std::string &req, size_t EndOfHeaders)
{
	std::string LineOfReq;
	size_t EndOfLine;
	char *splitedLine;
	size_t start = 0;
	size_t find = 0;
	std::string key;

	EndOfLine = req.find("\r\n");
	std::cout << "h = " << EndOfHeaders << "Start = " << start << "Er " << _Error << std::endl;
	while (EndOfHeaders > start && !_Error && EndOfLine != std::string::npos)
	{
		LineOfReq = req.substr(start, EndOfLine - start);
		if (!start)
		{
			char *line = &LineOfReq[0];
			splitedLine = std::strtok(line, " ");
			FillFirstLineInfo(splitedLine);
		}
		else
		{
			if ((find = LineOfReq.find(": ")) != std::string::npos)
			{
				key = trim(LineOfReq.substr(0, find));
				if (key.find(" ") != std::string::npos || (_headers.find("Host") != _headers.end() &&
														   (_headers["Host"].find(" ") != std::string::npos || key == "Host")))
					_Error = BAD_REQUEST;
				_headers[key] = trim(LineOfReq.substr(find + 2));
			}
			else
			{
				std::cout << "line of req = " << LineOfReq << std::endl;
				_Error = (!LineOfReq.empty()) ? BAD_REQUEST : _Error;
			}
		}
		start = EndOfLine + 2;
		EndOfLine = req.find("\r\n", EndOfLine + 2);
	}
	_buffer = _buffer.substr(EndOfHeaders + 4);
	this->_RequestCompleted = this->_Error;
	this->_HeadersCompleted = true;
};

void Request::FillFirstLineInfo(char *splitedLine)
{
	int i;
	for (i = 3; i > 0 && splitedLine != NULL; i--)
	{
		if (i == 3)
			this->_method = std::string(splitedLine);
		else if (i == 2)
		{
			std::string FindQuery(splitedLine);
			size_t found;
			if ((found = FindQuery.find("?")) != std::string::npos)
			{
				_path = FindQuery.substr(0, found);
				_query = FindQuery.substr(found + 1);
			}
			else
				_path = FindQuery;
		}
		else
			this->_version = std::string(splitedLine);
		splitedLine = std::strtok(NULL, " ");
	}
	this->_Error = (splitedLine != NULL || i > 0) ? BAD_REQUEST : _Error;
	this->_Error = (_method != "POST" && _method != "DELETE" && _method != "GET") ? BAD_REQUEST : _Error;
	this->_Error = (!_Error && _version.compare("HTTP/1.1")) ? HTTP_VERSION_NOT_SUPPORTED : _Error;
}

// Detected Server fonction
ServerCnf Request::_getValidServerCnf(std::vector<ServerCnf> const &serv_cnfs, struct sockaddr_in const addr)
{
	std::map<size_t, std::vector<std::string> > valid_cnf;
	std::map<size_t, std::vector<std::string> >::iterator it;
	std::map<std::string, std::string>::const_iterator h_it;

	// find which server block to use based on addr.host, addr.port

	for (size_t i = 0; i < serv_cnfs.size(); i++)
	{
		if (addr.sin_addr.s_addr == inet_addr(serv_cnfs[i].getHost().c_str()) && addr.sin_port == htons(serv_cnfs[i].getPort()))
			valid_cnf.insert(std::make_pair(i, serv_cnfs[i].getserver_names()));
	}
	if (valid_cnf.size() == 1)
		return serv_cnfs[valid_cnf.begin()->first];

	// if multiple servers are listening on the same host:port
	// check the "Host" header against server blocks' "server_names"

	std::string host = "";
	h_it = this->_headers.find("Host");
	if (h_it == _headers.end() || h_it->second.find(' ') != std::string::npos)
	{
		// 400 Bad Request
		_Error = BAD_REQUEST; // change it
		return serv_cnfs[0];
	}
	host = h_it->second;
	size_t idx = host.find(":");
	host = host.substr(0, idx != std::string::npos ? idx : host.size());
	for (it = valid_cnf.begin(); it != valid_cnf.end(); it++)
	{
		if (std::find(it->second.begin(), it->second.end(), host) != it->second.end())
			return serv_cnfs[it->first];
	}
	return serv_cnfs[0];
}

// max body size