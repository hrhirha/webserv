#include "Response.hpp"

Response::func Response::_req_func(std::string method)
{
	static std::map<std::string, func>	rf;

	if (rf.size()) return rf[method];

	rf["GET"] = &Response::_handleGetRequest;
	rf["POST"] = &Response::_handlePostRequest;
	rf["DELETE"] = &Response::_handleDeleteRequest;

	return rf[method];
}

std::string	specRes(size_t n)
{
	static std::map<int,std::string> sr;
	
	if (sr.size()) return sr[n];

	sr[301] = "<html>\r\n\
<head><title>301 Moved Permanently</title></head>\r\n\
<body>\r\n\
<center><h1>301 Moved Permanently</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[302]= "<html>\r\n\
<head><title>302 Found</title></head>\r\n\
<body>\r\n\
<center><h1>302 Found</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[307]= "<html>\r\n\
<head><title>307 Temporary Redirect</title></head>\r\n\
<body>\r\n\
<center><h1>307 Temporary Redirect</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[400] = "<html>\r\n\
<head><title>400 Bad Request</title></head>\r\n\
<body>\r\n\
<center><h1>400 Bad Request</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[403] = "<html>\r\n\
<head><title>403 Forbidden</title></head>\r\n\
<body>\r\n\
<center><h1>403 Forbidden</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[404] = "<html>\r\n\
<head><title>404 Not Found</title></head>\r\n\
<body>\r\n\
<center><h1>404 Not Found</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[405] = "<html>\r\n\
<head><title>405 Method Not Allowed</title></head>\r\n\
<body>\r\n\
<center><h1>405 Method Not Allowed</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	sr[500] = "<html>\r\n\
<head><title>500 Internal Server Error</title></head>\r\n\
<body>\r\n\
<center><h1>500 Internal Server Error</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>\r\n";

	return sr[n];
}

std::string statusMessage(size_t code)
{
	static std::map<size_t, std::string> sm;

	if (sm.size()) return sm[code];

	// 1×× Informational
	sm[100] = "Continue";
	sm[101] = "Switching Protocols";
	sm[102] = "Processing";

	//2×× Success
	sm[200] = "OK";
	sm[201] = "Created";
	sm[202] = "Accepted";
	sm[203] = "Non-authoritative Information";
	sm[204] = "No Content";
	sm[205] = "Reset Content";
	sm[206] = "Partial Content";
	sm[207] = "Multi-Status";
	sm[208] = "Already Reported";
	sm[226] = "IM Used";

	//3×× Redirection
	// sm[300] = "Multiple Choices";
	sm[301] = "Moved Permanently";
	sm[302] = "Moved Temporarily";
	sm[303] = "See Other";
	sm[304] = "Not Modified";
	// sm[305] = "Use Proxy";
	sm[307] = "Temporary Redirect";
	sm[308] = "Permanent Redirect";

	// 4×× Client Error
	sm[400] = "Bad Request";
	sm[401] = "Unauthorized";
	sm[402] = "Payment Required";
	sm[403] = "Forbidden";
	sm[404] = "Not Found";
	sm[405] = "Method Not Allowed";
	sm[406] = "Not Acceptable";
	sm[407] = "Proxy Authentication Required";
	sm[408] = "Request Timeout";
	sm[409] = "Conflict";
	sm[410] = "Gone";
	sm[411] = "Length Required";
	sm[412] = "Precondition Failed";
	sm[413] = "Payload Too Large";
	sm[414] = "Request-URI Too Long";
	sm[415] = "Unsupported Media Type";
	sm[416] = "Requested Range Not Satisfiable";
	sm[417] = "Expectation Failed";
	sm[418] = "I'm a teapot";
	sm[421] = "Misdirected Request";
	sm[422] = "Unprocessable Entity";
	sm[423] = "Locked";
	sm[424] = "Failed Dependency";
	sm[426] = "Upgrade Required";
	sm[428] = "Precondition Required";
	sm[429] = "Too Many Requests";
	sm[431] = "Request Header Fields Too Large";
	sm[444] = "Connection Closed Without Response";
	sm[451] = "Unavailable For Legal Reasons";
	sm[499] = "Client Closed Request";

	//5×× Server Error
	sm[500] = "Internal Server Error";
	sm[501] = "Not Implemented";
	sm[502] = "Bad Gateway";
	sm[503] = "Service Unavailable";
	sm[504] = "Gateway Timeout";
	sm[505] = "HTTP Version Not Supported";
	sm[506] = "Variant Also Negotiates";
	sm[507] = "Insufficient Storage";
	sm[508] = "Loop Detected";
	sm[510] = "Not Extended";
	sm[511] = "Network Authentication Required";
	sm[599] = "Network Connect Timeout Error";

	return sm[code];
}

std::string	mimeType(std::string s)
{
	static std::map<std::string,std::string> mt;

	if (mt.size())
	{
		std::string ext = s.substr(s.find_last_of('.')+1);
		std::map<std::string,std::string>::iterator it = mt.find(ext);
		return it != mt.end() ? it->second : "application/octet-stream";
	}

	mt["html"] = "text/html";
	mt["htm"] = "text/html";
	mt["shtml"] = "text/html";
	mt["css"] = "text/css";
	mt["xml"] = "text/css";
	mt["gif"]= "image/gif";
	mt["jpeg"]= "image/jpeg";
	mt["jpg"]= "image/jpeg";
	mt["js"]= "application/javascript";
	mt["atom"]= "application/atom+xml";
	mt["rss"]= "application/rss+xml";
	
	mt["mml"]= "text/mathml";
	mt["txt"]= "text/plain";
	mt["jad"]= "text/vnd.sun.j2me.app-descriptor";
	mt["wml"]= "text/vnd.wap.wml";
	mt["htc"]= "text/x-component";
	
	mt["png"]= "image/png";
	mt["tif"]= "image/tiff";
	mt["tiff"]= "image/tiff";
	mt["wbmp"]= "image/vnd.wap.wbmp";
	mt["ico"]= "image/x-icon";
	mt["jng"]= "image/x-jng";
	mt["bmp"]= "image/x-ms-bmp";
	mt["svg"]= "image/svg+xml";
	mt["svgz"]= "image/svg+xml";
	mt["webp"]= "image/webp";

	mt["woff"] = "application/font-woff";
	mt["jar"] = "application/java-archive";
	mt["war"] = "application/java-archive";
	mt["ear"] = "application/java-archive";
	mt["json"] = "application/json";
	mt["hqx"] = "application/mac-binhex40";
	mt["doc"] = "application/msword";
	mt["pdf"] = "application/pdf";
	mt["ps"] = "application/postscript";
	mt["eps"] = "application/postscript";
	mt["ai"] = "application/postscript";
	mt["rtf"] = "application/rtf";
	mt["m3u8"] = "application/vnd.apple.mpegurl";
	mt["xls"] = "application/vnd.ms-excel";
	mt["eot"] = "application/vnd.ms-fontobject";
	mt["ppt"] = "application/vnd.ms-powerpoint";
	mt["wmlc"] = "application/vnd.wap.wmlc";
	mt["kml"] = "application/vnd.google-earth.kml+xml";
	mt["kmz"] = "application/vnd.google-earth.kmz";
	mt["7z"] = "application/x-7z-compressed";
	mt["cco"] = "application/x-cocoa";
	mt["jardiff"] = "application/x-java-archive-diff";
	mt["jnlp"] = "application/x-java-jnlp-file";
	mt["run"] = "application/x-makeself";
	mt["pl"] = "application/x-perl";
	mt["pm"] = "application/x-perl";
	mt["prc"] = "application/x-pilot";
	mt["pdb"] = "application/x-pilot";
	mt["rar"] = "application/x-rar-compressed";
	mt["rpm"] = "application/x-redhat-package-manager";
	mt["sea"] = "application/x-sea";
	mt["swf"] = "application/x-shockwave-flash";
	mt["sit"] = "application/x-stuffit";
	mt["tcl"] = "application/x-tcl";
	mt["tk"] = "application/x-tcl";
	mt["der"] = "application/x-x509-ca-cert";
	mt["pem"] = "application/x-x509-ca-cert";
	mt["cert"] = "application/x-x509-ca-cert";
	mt["xpi"] = "application/x-xpinstall";
	mt["xhtml"] = "application/xhtml+xml";
	mt["xspf"] = "application/xspf+xml";
	mt["zip"] = "application/zip";

	mt["bin"] = "application/octet-stream";
	mt["exe"] = "application/octet-stream";
	mt["dll"] = "application/octet-stream";
	mt["deb"] = "application/octet-stream";
	mt["dmg"] = "application/octet-stream";
	mt["iso"] = "application/octet-stream";
	mt["img"] = "application/octet-stream";
	mt["msi"] = "application/octet-stream";
	mt["msp"] = "application/octet-stream";
	mt["msm"] = "application/octet-stream";

	mt["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	mt["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	mt["pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";

	mt["mid"] = "audio/midi";
	mt["midi"] = "audio/midi";
	mt["kar"] = "audio/midi";
	mt["mp3"] = "audio/mpeg";
	mt["ogg"] = "audio/ogg";
	mt["m4a"] = "audio/x-m4a";
	mt["ra"] = "audio/x-realaudio";

	mt["3gpp"] = "video/3gpp";
	mt["3gp"] = "video/3gpp";
	mt["ts"] = "video/mp2t";
	mt["mp4"] = "video/mp4";
	mt["mpeg"] = "video/mpeg";
	mt["mpg"] = "video/mpeg";
	mt["mov"] = "video/quicktime";
	mt["webm"] = "video/webm";
	mt["flv"] = "video/x-flv";
	mt["m4v"] = "video/x-m4v";
	mt["mng"] = "video/x-mng";
	mt["asx"] = "video/x-ms-asf";
	mt["asf"] = "video/x-ms-asf";
	mt["wmv"] = "video/x-ms-wmv";
	mt["avi"] = "video/x-msvideo";

	std::string ext = s.substr(s.find_last_of('.') + 1);
	std::map<std::string, std::string>::iterator it = mt.find(ext);
	return it != mt.end() ? it->second : "application/octet-stream";
}

std::string utostr(size_t n)
{
	std::stringstream	ss;
	std::string			str;

	ss << n;
	ss >> str;

	return str;
}

size_t strtou(std::string s)
{
	std::stringstream	ss;
	size_t				n;

	ss << s;
	ss >> n;

	return n;
}

std::string	strtoupper(std::string s)
{
	for (size_t i = 0; i < s.size(); i++)
		s[i] = toupper(s[i]);
	return s;
}

std::string timeToStr(time_t clock, bool lst)
{
	struct tm	*tm;

	tm = gmtime(&clock);
	// std::string str = asctime(tm);
	if (lst)
	{
		char time[18];
		strftime(time, 18, "%d-%b-%Y %H:%M", tm);
		return std::string(time);
	}
	char time[30];
	strftime(time, 30, "%a, %d %b %y %H:%M:%S GMT", tm);
	return std::string(time);
}

char **vectorToPtr(std::vector<std::string> &v)
{
	char **ptr = new char*[v.size() + 1];
	for (size_t i = 0; i < v.size(); i++)
	{
		ptr[i] = new char[v[i].length()+1];
		strcpy(ptr[i], v[i].c_str());
	}
	ptr[v.size()] = NULL;
	return ptr;
}

Headers strToHeaders(char *str)
{
	Headers hs;
	char	*arg;

	arg = strtok(str, "\r\n");
	while (arg)
	{
		std::string  line = arg;
		size_t col = line.find(":");
		if (col != std::string::npos)
		{
			std::string key = line.substr(0, col);
			std::string val = line.substr(col+1);
			val = val.substr(val.find_first_not_of(" "), val.find_last_not_of(" ")+1);
			hs[key] = val;
		}
		arg = strtok(NULL, "\r\n");
	}
	return hs;
}

std::string getHyperlinkTag(std::string &name, struct stat &st)
{
	std::string a;
	std::string lst_mod = timeToStr(st.st_mtime, true);
	std::string sz = (S_ISREG(st.st_mode)) ? utostr(st.st_size) : "-";

	a.append("<a href=\""+name+"\">"+(name.size() > 50
		? name.substr(0, 47)+"..&gt;"
		: name)+"</a> "+(name.size() <= 50 ? std::string(50 - name.size(), ' ') : ""));
	a.append(lst_mod);
	a.append(20-sz.size(), ' ');
	a.append(sz);
	a.append("\n");
	return a;
}

void freePtr(char **ptr)
{
	for (int i = 0; ptr[i]; i++)
		delete [] ptr[i];
	delete [] ptr;
}
