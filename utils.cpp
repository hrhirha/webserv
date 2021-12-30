#include "Response.hpp"

std::string	Response::_spec_res(size_t n)
{
	static std::map<int,std::string> sr;
	
	if (sr.size()) return sr[n];

	sr[301] = "<html>\r\n\
<head><title>301 Moved Permanently</title></head>\r\n\
<body>\r\n\
<center><h1>301 Moved Permanently</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>";

	sr[400] = "<html>\r\n\
<head><title>400 Bad Request</title></head>\r\n\
<body>\r\n\
<center><h1>400 Bad Request</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>";

	sr[403] = "<html>\r\n\
<head><title>403 Forbidden</title></head>\r\n\
<body>\r\n\
<center><h1>403 Forbidden</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>";

	sr[404] = "<html>\r\n\
<head><title>404 Not Found</title></head>\r\n\
<body>\r\n\
<center><h1>404 Not Found</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>";

	sr[405] = "<html>\r\n\
<head><title>405 Method Not Allowed</title></head>\r\n\
<body>\r\n\
<center><h1>405 Method Not Allowed</h1></center>\r\n\
<hr><center>webserv/1.0</center>\r\n\
</body>\r\n\
</html>";

	return sr[n];
}

std::string	Response::_mime_type(std::string s)
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