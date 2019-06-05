/**
 * @file    exception.cc
 * @author  Nader KHAMMASSI
 * @contact nader.khammassi@gmail.com
 * @date    15/03/2010
 */




/**
 * exception implementation
 */

exception::exception(const std::string &message, 
                     bool system_message)
                     throw() : user_message(message) 
{
  if (system_message) 
  {
    user_message.append(": ");
    user_message.append(strerror(errno));
  }
}

/**
 * dtor
 */

exception::~exception() throw() 
{
}

/**
 * explainatory message
 */

const char *exception::what() const throw() 
{
  return user_message.c_str();
}

