/**
 * @file        str.h
 * @author	Nader KHAMMASSI - nader.khammassi@gmail.com 
 * @date	20-01-12
 * @copyright  
 * 
 *  XPU - C++ Parallel  Programming Library for Multicore Architectures
 *
 *  Copyright (C) 2014 Nader Khammassi, All Rights Reserved.
 *
 * This file is part of XPU and has been downloaded from 
 * http://ww.xpu-project.net/.
 *
 * XPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * XPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * @brief 
 *    string utils 
 */

#ifndef XPU_STRINGS_H
#define XPU_STRINGS_H


#include <vector>
#include <string.h>
#include <sstream>

namespace str
{
   typedef std::vector<std::string> strings;

   /**
    * @param str 
    *    string to be processed
    * @param seq 
    *    string to be replaced
    * @param rep 
    *    string used to replace seq
    * @brief
    *    replace recursively seq by rep in str
    */
   inline void replace_all(std::string &str, std::string seq, std::string rep)
   {
      size_t index = str.find(seq);
      while (index < str.size())
      {
	 str.replace(index, seq.size(), rep);
	 index = str.find(seq);
      }
   }

   /**
    * @param str 
    *    string to be processed
    * @param seq 
    *    sequence witch we want count 
    *    occurences
    * @return 
    *    occurence count of seq in str
    */
   inline size_t occur_count(std::string &str, std::string seq)
   {
      int count = 0, index = str.find(seq);
      while (index < str.size()) // && index >= 0 )
      {
	 count++;
	 index = str.find(seq, index+1);
      }
      return count;
   }



   /**
    * @param str 
    *    string to be processed
    * @param separator
    *    words seprator (usually space charachter but can be any string) 
    * @return 
    *    word list of a given string. 
    */
   inline strings word_list(std::string &str, std::string separator)
   {
      strings wrds;
      size_t  index = str.find(separator);
      size_t  prev = 0;
      while (index < str.size()) // && index >= 0 )
      {
	 wrds.push_back(str.substr(prev,index-prev));
	 prev=index+1;
	 index = str.find(separator, index+1);
      }
      wrds.push_back(str.substr(prev));
      return wrds;
   }

   /**
    * @param str 
    *    string to be processed
    * @brief
    *    convert string to lower case
    */
   inline void lower_case(std::string& str)
   {
      for (size_t i=0; i<str.size(); ++i)
      {
	 char c = str[i];
	 if(c<='Z' && c>='A')
	    str[i] = c-('Z'-'z');
      }
   }

   /**
    * @param str 
    *    string to be processed
    * @brief
    *    code line format : words separated by spaces : 
    *    "word word word ...")
    */
   inline void format_line(std::string &line)
   {
      lower_case(line);
      replace_all(line, "\t", " ");
      replace_all(line, "\n", " ");
      replace_all(line, "  ", " ");
      replace_all(line, ", ", ",");
      replace_all(line, " ,", ",");
      replace_all(line, " , ", ",");

      if (line[0] == ' ')
	 line.erase(0, 1);
      if (line[line.size()-1] == ' ')
	 line.erase(line.size()-1, 1);
   }

   /**
    * @param str 
    *    string to be processed
    * @brief
    *    remove inline comment 
    */
   inline void remove_comment(std::string &line, char c='#')
   {
      size_t p = line.find(c);
      if (p != std::string::npos)
	 line = line.substr(0,p); 
   }


   /**
    * @param str 
    *    string to be processed
    * @return 
    *    word count in a line (line format : words 
    *    separated by spaces : "word word word ...")
    */
   inline size_t word_count(std::string &str)
   {
      // spaces count
      size_t count = occur_count(str, " ");
      return count+1;
   }


   /**
    * return true is x is a space/return/tab... charcter
    */
   inline bool is_space(char x)
   {
      switch (x)
      {
	 case '\f' : return true; 
	 case '\n' : return true; 
	 case '\r' : return true; 
	 case '\t' : return true; 
	 case '\v' : return true; 
	 case ' '  : return true; 
	 default   : return false; 
      }
   }

   /**
    * is digit
    */
   inline bool is_digit(char x)
   {
      return ((x >= '0') && (x<='9'));
   }

   /**
    * is letter
    */
   inline bool is_letter(char x)
   {
      return (((x >= 'a') && (x<='z')) || ((x >= 'A') && (x<='Z')) || (x == '_'));
   }
   
   /**
    * is dot
    */
   inline bool is_dot(char x)
   {
      return (x=='.');
   }

   /**
    * is period
    */
   inline bool is_period(char x)
   {
      return (x==';');
   }

   /**
    * @param str
    *    string to be processed
    * @return 
    *    true if 'str' do not contain any letter or number
    */
   inline bool is_empty(std::string &str)
   {
      for (int i=0; i<str.size(); ++i)
      {
	 char c = str[i];
	 if (is_letter(c) || is_digit(c))
	    return false;
      }
      return true;
   }



   /**
    * int to string
    */
   std::string int_to_str(int i)
   {
      std::stringstream strm;
      std::string num;
      strm << i;
      strm >> num;
      return num;
   }

}


#endif // XPU_STRINGS_H

