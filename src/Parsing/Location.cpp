/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/01/03 16:16:13 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/01/10 16:53:44 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Location.hpp"

/*			Constructor 			*/
	Location::Location( void ) : _Err(false),_pathOfLocation(""),
	_location_root(""), _index(""), _autoindex(false), _upload_path(""), _pathcgi("")
	{};

/*			Destructor 				*/
	Location::~Location( void )
	{};

/*			Copy Constructor		*/
	Location::Location ( const Location& loca )
	{	*this = loca;	};

/*			Operator= 				*/
	Location& Location::operator= ( const Location& loca)
	{
		// _LocaCompleted		= loca._LocaCompleted;
		_pathOfLocation		= loca.getPathOfLocation();
		_location_root		= loca.getLocation_root();
		_index				= loca.getIndex();
		_autoindex			= loca.getAutoIndex();
		_upload_path		= loca.getUpload_path();
		_pathcgi			= loca.getPathCgi();
		_accepted_methods	= loca.getAcceptedMethods();
		_redirect			= loca.getRedirect();
		_Err				= loca.getErr(); 
		return *this;
	};

/*			Getters					*/
	bool	Location::getErr( void ) const
	{	return _Err;				}
	Methods Location::getAcceptedMethods( void ) const 
	{	return	_accepted_methods;	};
	Red			Location::getRedirect( void ) const
	{	return	_redirect;			};
	bool			Location::getAutoIndex( void ) const
	{	return	_autoindex;			};
	std::string	Location::getPathOfLocation( void ) const
	{	return	_pathOfLocation;	};
	std::string	Location::getLocation_root( void ) const
	{	return	_location_root;		};
	std::string	Location::getIndex( void ) const
	{	return	_index;				};
	std::string	Location::getUpload_path( void ) const
	{	return	_upload_path;		};
	std::string	Location::getPathCgi( void ) const
	{	return	_pathcgi;			};

/*			fill location	*/
	Location&	Location::parseLocation( std::ifstream& _ifs, std::string& path)
	{
		_pathOfLocation = (path[path.size() - 1] == '/' || path[0] == '.') ? path : path.append("/"); // modified by hamza
		std::string line;
		while (std::getline(_ifs, line))
		{
			std::vector<std::string> SplitedVec = split(line, ' ');
			size_t VecSize = SplitedVec.size();
			if (!VecSize)
				continue;
			else if (SplitedVec[0] == "]" && VecSize == 1)
				return *this;
			else if (SplitedVec[0] == "root" && VecSize == 2)
				_location_root = SplitedVec[1];
			else if (SplitedVec[0] == "cgi" && VecSize == 2)
				_pathcgi = SplitedVec[1];
			else if (SplitedVec[0] == "index" && VecSize == 2)
				_index = SplitedVec[1];
			else if (SplitedVec[0] == "upload_path" && VecSize == 2)
				_upload_path = SplitedVec[1];
			else if (SplitedVec[0] == "return" && VecSize == 3)
			{
				_redirect.first = (isStrDigit(SplitedVec[1])) ? to_sizeT(SplitedVec[1]) : (_Err = 1);
				_redirect.second = SplitedVec[2];
			}
			else if (SplitedVec[0] == "autoindex" && VecSize == 2 && SplitedVec[1] == "on")
				_autoindex = true;
			else if (SplitedVec[0] == "allowed_method" && VecSize > 1)
				for (size_t i = 1; i < VecSize; i++)
					_accepted_methods.push_back(SplitedVec[i]);
		}
		return *this; 
	}