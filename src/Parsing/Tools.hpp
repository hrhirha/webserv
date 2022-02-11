/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tools.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/02/11 17:58:53 by ibouhiri          #+#    #+#             */
/*   Updated: 2022/02/11 18:00:43 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOOLS_HPP
#define TOOLS_HPP

# include <arpa/inet.h>
# include <iostream>
# include <cstdlib>
# include <cstring>
# include <map>
# include <vector>
# include <utility>
# include <fstream>
# include <sstream>
# include <sys/time.h>
# include <math.h>
# include <ctime>
# include <algorithm>

// helpers Fonctions
std::string trim(const std::string &s);
size_t	to_sizeT(std::string str);
int convert(char num[]);
std::vector<std::string> split (std::string line, char delimiter);
bool isStrDigit (const std::string& str);

#endif