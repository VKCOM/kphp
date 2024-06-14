// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC Sigmalab
// Distributed under the GPL v3 License, see LICENSE.notice.txt

//static std::list<string> static_envs;
#include "env_putenv.h"

int f$putenv(const string& env)
{
   //TODO: find same declaration to avoid duplicating
   const char *s = strchr(env.c_str(), '=');
   if ( s ) {
      //php_assert (s != nullptr);
      string name = string(env.c_str(), static_cast<string::size_type>(s - env.c_str()));
      string value= string(s + 1);
      return setenv(name.c_str(), value.c_str(), 1);
   }
//	static_envs.push_back( env.c_str() );
//	const string& ref = static_envs.back();
//	return putenv((char*)ref.c_str());
   return EINVAL;
}

