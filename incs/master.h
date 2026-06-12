/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   master.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jleal <jleal@student.42lisboa.com>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 19:37:03 by jleal             #+#    #+#             */
/*   Updated: 2026/06/08 20:10:54 by htrindad         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <poll.h>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <fcntl.h>
#include <limits.h>

#define RED "\e[1;31m"
#define WHI "\e[0;37m"
#define GRE "\e[1;32m"
#define YEL "\e[1;33m"

#define PORT_MAX 0xffff // basically USHRT_MAX in case someone's short is not really a 16-bit number.
