/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCnf.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/01/04 12:16:11 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/02/12 15:41:13 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerCnf.hpp"
#include <cerrno>

/*			Constructor 			*/
ServerCnf::ServerCnf(void) : _host("0.0.0.0"), _port(80),
							 _client_max_body_size(-1), _fillSrvCompleted(false){};

/*			Parameters Constructor 	*/
ServerCnf::ServerCnf(const std::string &file) : _host("0.0.0.0"), _port(80),
												_client_max_body_size(-1), _fillSrvCompleted(false)
{
	_ifs.open(file.c_str(), std::ifstream::in);
	if (_ifs.good())
		parse();
	else
	{
		std::cout << strerror(errno) << std::endl;
		exit(-1);
	}
};

/*			Destructor 				*/
ServerCnf::~ServerCnf(void){};

/*			Copy Constructor		*/
ServerCnf::ServerCnf(const ServerCnf &serv)
{
	*this = serv;
};

/*			Operator= 				*/
ServerCnf &ServerCnf::operator=(const ServerCnf &serv)
{
	_host = serv.getHost();
	_port = serv.getPort();
	_server_names = serv.getserver_names();
	_client_max_body_size = serv.getclient_max_body_size();
	_locs = serv.getlocs();
	_error_pages = serv.geterror_pages();
	_srvs = serv.getServers();

	return *this;
};

/*			Getters					*/
std::string ServerCnf::getHost(void) const
{
	return _host;
};
size_t ServerCnf::getPort(void) const
{
	return _port;
};
std::vector<std::string> ServerCnf::getserver_names(void) const
{
	return _server_names;
};
size_t ServerCnf::getclient_max_body_size(void) const
{
	return _client_max_body_size;
};
Locations ServerCnf::getlocs(void) const
{
	return _locs;
};
error ServerCnf::geterror_pages(void) const
{
	return _error_pages;
};
std::vector<ServerCnf> ServerCnf::getServers(void) const
{
	return _srvs;
};

/*			Error Print 			*/
size_t ServerCnf::ErrorPrint(void)
{
	std::cout << "Error in config file please fixe it." << std::endl;
	_ifs.close();
	exit(0);
	return (1);
}

/*			parse Fonctions			*/
void ServerCnf::parse(void)
{
	std::string line;
	ServerCnf inst;
	while (std::getline(_ifs, line))
	{
		if (!_fillSrvCompleted && line.empty())
			continue;
		if (!_fillSrvCompleted)
		{
			_fillSrvCompleted = true;
			inst = ServerCnf();
		}
		fillServer(line, inst);
		if (!inst._fillSrvCompleted)
			continue;
		if (inst._host == "localhost")
			inst._host = "127.0.0.1";
		_srvs.push_back(inst);
		_fillSrvCompleted = false;
	}
	if (_fillSrvCompleted || !_srvs.size())
		ErrorPrint();
	// printInstance();
	_ifs.close();
};
/*			Print Instance 			*/
void ServerCnf::printInstance(void)
{
	std::cout << "*****************[START]*****************" << std::endl;
	for (size_t serv = 0; serv < _srvs.size(); serv++)
	{
		std::cout << "inst._host = [" << _srvs[serv]._host << "]" << std::endl;
		std::cout << "**********************************" << std::endl;
		std::cout << "_srvs[" << serv << "]._port = [" << _srvs[serv]._port << "]" << std::endl;
		std::cout << "**********************************" << std::endl;
		std::cout << "_srvs[" << serv << "]._client_max_body_size = [" << _srvs[serv]._client_max_body_size << "]" << std::endl;
		std::cout << "**********************************" << std::endl;
		for (size_t k = 0; k < _srvs[serv]._error_pages.first.size(); k++)
			std::cout << "_srvs[" << serv << "]._error_pages = [" << k << "] = " << _srvs[serv]._error_pages.first[k] << std::endl;
		std::cout << "_srvs[" << serv << "]._error_pages = ["
				  << "PATH"
				  << "] = " << _srvs[serv]._error_pages.second << std::endl;
		std::cout << "**********************************" << std::endl;
		for (size_t k = 0; k < _srvs[serv]._server_names.size(); k++)
			std::cout << "_srvs[" << serv << "].server_name = [" << k << "] = " << _srvs[serv]._server_names[k] << std::endl;
		std::cout << "**************[Locations]*************" << std::endl;
		for (size_t k = 0; k < _srvs[serv]._locs.size(); k++)
		{
			std::cout << "_srvs[" << serv << "]._locs[" << k << "]._autoindex = [" << _srvs[serv]._locs[k].getAutoIndex() << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			std::cout << "_srvs[" << serv << "]._locs[" << k << "]._index = [" << _srvs[serv]._locs[k].getIndex() << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			std::cout << "_srvs[" << serv << "]._locs[" << k << "].getPathCgi = [" << _srvs[serv]._locs[k].getPathCgi() << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			std::cout << "_srvs[" << serv << "]._locs[" << k << "].getUpload_path = [" << _srvs[serv]._locs[k].getUpload_path() << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			std::cout << "_srvs[" << serv << "]._locs[" << k << "].getPathOfLocation = [" << _srvs[serv]._locs[k].getPathOfLocation() << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			std::cout << "_srvs[" << serv << "]._locs[" << k << "].getLocation_root = [" << _srvs[serv]._locs[k].getLocation_root() << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			std::cout << "_srvs[" << serv << "]._locs[" << k << "]._redirection = Nmbr redi = [" << _srvs[serv]._locs[k].getRedirect().first << "]"
					  << " | Path [" << _srvs[serv]._locs[k].getRedirect().second << "]" << std::endl;
			std::cout << "**********************************" << std::endl;
			for (size_t i = 0; i < _srvs[serv]._locs[k].getAcceptedMethods().size(); i++)
				std::cout << "_srvs[" << serv << "]._locs.acceptedMethods = [" << i << "] = " << _srvs[serv]._locs[k].getAcceptedMethods()[i] << std::endl;
		}
		std::cout << "*****************[END]*****************" << std::endl;
	}
}

void ServerCnf::fillServer(std::string &line, ServerCnf &inst)
{
	// std::cout << "line [ " << line << "]"<< std::endl;
	std::vector<std::string> SplitedVec = split(line, ' ');
	size_t VecSize = SplitedVec.size();
	for (size_t i = 0; i < VecSize; i++)
	{
		if (SplitedVec[i] == "server" && VecSize == 2 && SplitedVec[i + 1] == "{")
			inst._fillSrvCompleted = (i++) ? false : false;
		else if (SplitedVec[i] == "}" && VecSize == 1)
			inst._fillSrvCompleted = true;
		else if (SplitedVec[i] == "host" && VecSize == 2)
			inst._host = SplitedVec[++i];
		else if (SplitedVec[i] == "port" && VecSize == 2)
			inst._port = (isStrDigit(SplitedVec.back())) ? to_sizeT(SplitedVec[++i]) : ErrorPrint();
		else if (SplitedVec[i] == "client_max_body_size" && VecSize == 2)
			inst._client_max_body_size = (isStrDigit(SplitedVec.back())) ? to_sizeT(SplitedVec[++i]) : ErrorPrint();
		else if (SplitedVec[i] == "error_pages" && VecSize > 2) /* Fill error pages and Path */
			for (++i; i < VecSize; i++)
				if ((i + 1) < VecSize && isStrDigit(SplitedVec[i]))
					inst._error_pages.first.push_back(to_sizeT(SplitedVec[i]));
				else if ((i + 1) < VecSize && !isStrDigit(SplitedVec[i]))
					ErrorPrint();
				else
					inst._error_pages.second = SplitedVec[i];
		else if (SplitedVec[i] == "server_name" && VecSize > 1) /* Fill servers names */
			for (++i; i < VecSize; i++)
				inst._server_names.push_back(SplitedVec[i]);
		else if (SplitedVec[i] == "location" && VecSize == 3)
		{
			inst._locs.push_back(Location().parseLocation(_ifs, SplitedVec[i + 1]));
			if (inst._locs.back().getErr())
				ErrorPrint();
			return;
		}
		else
		{
			std::cout << "Error [" << SplitedVec[0] << "]" << std::endl;
			ErrorPrint();
		}
	}
	return;
};
