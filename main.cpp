#include "Response.hpp"
#include <sys/stat.h>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

Location newLocation(std::string path, vs methods, std::pair<size_t,std::string> redirect, std::string root, std::string index, bool autoindex)
{
	Location loc;
	loc.path = path;
	loc.accepted_methods = methods;
	loc.redirect = redirect;
	loc.root = root;
	loc.index = index;
	loc.autoindex = autoindex;
	return loc;
}

ServerCnf newServer(std::string host, size_t port, vs server_names, Locations locs)
{
	ServerCnf srv;
	srv.host = host;
	srv.port = port;
	srv.server_names = server_names;
	srv.locs = locs;
	return srv;
}

int main()
{
	std::vector<ServerCnf>	srvs;
	Locations				ls;
	Headers					hs;
	vs						sns;
	Response				res;
	struct sockaddr_in		addr;

	// server 0
	ls.push_back(newLocation("/", vs(), std::make_pair(0,""), "", "", false));
	sns.push_back("");
	srvs.push_back(newServer("127.0.0.1", 8000, sns, ls));
	//////////
	ls.clear();
	hs.clear();
	sns.clear();
	// server 1
	ls.push_back(newLocation("/", vs(), std::make_pair(0,""), "html", "index.html", false));
	ls.push_back(newLocation("/dir0/", vs(), std::make_pair(0,""), "html", "f0", false));
	ls.push_back(newLocation("/dir0/dir00/", vs(), std::make_pair(0,""), "html", "f0", false));
	ls.push_back(newLocation(".php", vs(), std::make_pair(0,""), "html", "f0", false));
	sns.push_back("localhost");
	srvs.push_back(newServer("127.0.0.1", 8000, sns, ls));
	//////////
	ls.clear();
	hs.clear();
	sns.clear();
	// server 2
	ls.push_back(newLocation("/", vs(), std::make_pair(0,""), "html/dir1", "index.html", false));
	sns.push_back("test.com");
	sns.push_back("test.net");
	srvs.push_back(newServer("127.0.0.1", 8000, sns, ls));
	/////////

	hs.insert(std::make_pair("Host", "localhost"));
	Request req = {"GET", "/", "", "HTTP/1.1", hs, ""};

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8000);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	res.build(req, srvs, addr);
	std::cout << "--------------------------------------\n";
	std::cout << res.toString() << std::endl;
	std::cout << "--------------------------------------\n";


	struct stat buf;
	std::string path = "/Users/hrhirha/Desktop/wserver/www///index.html/";
	if (stat(path.c_str(), &buf) == 0 && (buf.st_mode & S_IFMT) == S_IFREG)
	{
		int fd;
		std::cout << "file found" << std::endl;
		if ((fd = open(path.c_str(), O_RDONLY)) == -1)
			std::cout << "Error (open()): " << errno << std::endl;
		else
			close(fd);
	}
	else
		std::cout << "Error (stat()): " << errno << std::endl;
		std::cout << "ENOENT: " << ENOENT << std::endl;
		std::cout << "ENOTDIR: " << ENOTDIR << std::endl;

}
