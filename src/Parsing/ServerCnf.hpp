/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCnf.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/01/04 12:16:09 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/02/17 12:07:44 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCNF_HPP
#define SERVERCNF_HPP

#include "Location.hpp"

typedef std::vector<size_t>							error_pages_code;
typedef std::pair<error_pages_code, std::string>	error;
typedef std::vector<Location>						Locations;

class ServerCnf
{
	private:
	/*			Attributs			*/
		bool							_checkPort;
		std::string						_host;
		size_t							_port;
		std::vector<std::string>		_server_names;
		size_t							_client_max_body_size;
		Locations						_locs; // locations in server
		error							_error_pages;
		std::ifstream					_ifs;
		bool							_fillSrvCompleted;
		std::vector<ServerCnf>			_srvs; // servers 
		
	public:
	/*			Constructors		*/
		ServerCnf( void );
		ServerCnf( const std::string& file );
		~ServerCnf( void );
		ServerCnf(const ServerCnf& serv );
		ServerCnf& operator= (const ServerCnf& serv );
	
	/*			Getters				*/
		std::string					getHost( void ) const;
		size_t						getPort( void ) const;
		std::vector<std::string>	getserver_names( void ) const;
		size_t						getclient_max_body_size( void ) const;
		Locations					getlocs( void ) const;
		error						geterror_pages( void ) const;
		std::vector<ServerCnf>		getServers( void ) const;

	/*			Print Fonctions 	*/
		void	printInstance(void);
		size_t	ErrorPrint(void);
	
	/*			Parse Fonctions		*/
		void	parse(void);
		void	fillServer(	std::string& line, ServerCnf& instace);
};

#endif