/**
 * @file    exception.h
 * @author  Nader KHAMMASSI
 * @contact nader.khammassi@gmail.com
 * @date    15/03/2010
 * @brief   ql exception (from xpu)
 *
 * @copyright
 *
 * XPU - C++ Parallel  Programming Library for Multicore Architectures
 *
 * Copyright (C) 2014 Nader Khammassi, All Rights Reserved.
 *
 * This file is part of QL and has been downloaded from
 * http://www.ql-project.net/.
 *
 * QL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * QL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#ifndef QL_EXCEPTION_H
#define QL_EXCEPTION_H

#include <string>            // for string
#include <cstring>           // for strerror
#include <exception>         // for exception
#include <cerrno>            // for errno

namespace ql
{
   class exception : public std::exception
   {

	 public:

	   /**
	    *   construct a exception with a explanatory message.
	    *
	    *   @param message         explanatory message
	    *   @param system_message  true if system message (from strerror(errno))
	    *                          should be postfixed to the user provided message
	    */
	   inline exception(const std::string &message, bool system_message = false) throw();

	   /**
	    *   provided just to guarantee that no exceptions are thrown.
	    */
	   inline ~exception() throw();

	   /**
	    *   get the exception message
	    *   @return exception message
	    */
	   inline const char *what() const throw();

	 private:

	   std::string user_message;  // exception message

   }; // class exception

   #include "exception.cc"  // FIXME

} // namespace ql

#endif // QL_EXCEPTION_H


