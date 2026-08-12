#ifndef UTILS_HPP
# define UTILS_HPP

#include "type.hpp"
#include <netinet/in.h>
#include <string>

bool nonblockingSet(int fd);

EnvMap envpParsing(char **envp);

char **mapToEnvp(const EnvMap &env);

void freeSplit(char **tmp);

std::string ipToString(in_addr_t addr);

#endif