/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: htrindad <htrindad@student.42lisboa.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/08 17:58:06 by htrindad          #+#    #+#             */
/*   Updated: 2026/06/08 20:14:02 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <master.h>

class Message
{
	private:
		std::string			message;
		std::string			prefix;
		std::string			command;
		std::vector<std::string>	params;
		std::string			middle;
		std::string			trailing;
	public:
		Message();
		~Message();
		std::string			getMessage() const;
		std::string			getPrefix() const;
		std::string			getCommand() const;
		std::vector<std::string>	getParams() const;
		std::string			getMiddle() const;
		std::string			getTrailing() const;
		void				setMessage(const std::string &msg);
		void				setPrefix(const std::string &pre);
		void				setCommand(const std::string &cmd);
		void				setParams(const std::vector<std::string> &par);
		void				setMiddle(const std::string &mid);
		void				setTrailing(const std::string &trail);
		static Message			parse(const std::string &raw, const std::vector<std::string> &cmdLst);
};
