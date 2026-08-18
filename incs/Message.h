/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mely-pan <mely-pan@student.42lisboa.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 17:58:06 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/19 18:29:08 by mely-pan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>

/*
enum conditions
{
	NONE,	// undefined
	NONTSI,	// takes an optional parameter
	SINGLE,	// takes a single parameter
	STDBLE, // short for `SINGLE_TO_DOUBLE`. Takes from 1 to 2 parameters
	DOUBLE,	// takes 2 parameters
	DBLTRI,	// short for `DOUBLE_TRIPLE`. Takes 2 to 3 parameters
	QUAD,	// takes 4 parameters
	DTQUAD,	// Takes a range of 2 to 4 parameters
};
*/

class Message
{
	private:
		std::string			message;
		std::string			command;
		std::vector<std::string>	params;
		std::string			middle;
		std::string			trailing;
	public:
		Message();
		~Message();
		const std::string				&getMessage() const;
		const std::string				&getCommand() const;
		const std::vector<std::string>	&getParams() const;
		const std::string			&getMiddle() const; // target
		const std::string			&getTrailing() const;
		const std::string	&getParam(size_t i) const;
		bool				hasParam(size_t i) const;
		size_t				paramCount() const;
		void				setMessage(const std::string &msg);
		void				setCommand(const std::string &cmd);
		void				setParams(const std::vector<std::string> &par);
		void				setMiddle(const std::string &mid);
		void				setTrailing(const std::string &trail);
		static Message			parse(const std::string &raw);
};
