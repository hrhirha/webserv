/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/12/21 09:24:34 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/02/12 16:01:08 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "ServerCnf.hpp"

typedef std::map<std::string, std::string> Headers;

class Request
{
private:
	std::vector<ServerCnf> _srvs;
	struct sockaddr_in _addr;
	ServerCnf _ServerBlock;
	std::string _buffer;
	std::string _method;
	std::string _path;
	std::string _query;
	std::string _version;
	Headers _headers;
	std::string _body;
	bool _requestCompleted;
	bool _headersCompleted;
	size_t _Error;
	size_t _BodySize;
	size_t _HowMuchShouldRead;
	std::ofstream _fstream;

public:
	// canonical form
	Request(std::vector<ServerCnf> srvs, struct sockaddr_in addr);
	Request();
	Request(Request const &copy);
	Request &operator=(Request const &copy);
	~Request(void);

	// getters
	ServerCnf &getServerBlock(void);
	std::string &getmethod(void);
	std::string &getpath(void);
	std::string &getquery(void);
	std::string &getversion(void);
	std::string &getbody(void);
	Headers &getheaders(void);
	bool isRequestCompleted(void);

	size_t getError(void) { return _Error; }

	// print content method
	void print(void);
	std::string generateNameFile(void);

	// Parse Methods
	void Parse(std::string &req);
	void parseHeaders(std::string &req, size_t EndOfHeaders);
	void FillFirstLineInfo(char *splitedLine);
	void FillBody(std::string &req);
	void ChunkedRequest(std::string &req);
	// Detected Server Fonction
	ServerCnf _getValidServerCnf(std::vector<ServerCnf> const &serv_cnfs, struct sockaddr_in const addr);


	void printll();
};

#endif