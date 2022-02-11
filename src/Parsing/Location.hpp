/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/01/03 16:15:12 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/02/11 18:01:05 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "Tools.hpp"

typedef std::vector<std::string>		Methods;
typedef std::pair<size_t, std::string>	Red;

class Location
{
	private:
	/*			Attributs		*/
		bool			_Err;
		std::string		_pathOfLocation; // *
		std::string		_location_root; // *
		std::string		_index; //*
		bool			_autoindex; // *
		std::string		_upload_path;//*
		std::string		_pathcgi; //*
		Methods			_accepted_methods; //*
		Red				_redirect;

	public:
	/*			Constructors	*/
		Location ( void );
		Location ( const Location& copy );
		~Location ( void );
		Location& operator= ( const Location& loca );

	/*			Getters			*/
		bool		getErr( void ) const;
		Methods		getAcceptedMethods( void ) const;
		Red			getRedirect( void )	const;
		bool		getAutoIndex( void ) const;
		std::string	getPathOfLocation( void ) const;
		std::string	getLocation_root( void ) const;
		std::string	getIndex( void ) const;
		std::string	getUpload_path( void ) const;
		std::string	getPathCgi( void ) const;
	/*			fill location	*/
		Location&		parseLocation( std::ifstream& _ifs, std::string& path);
};

#endif
