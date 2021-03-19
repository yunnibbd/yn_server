#include "cabstract_server.h"
#include <string.h>

void CAbstractServer::set_server_ip(const char *ip)
{
	strncpy(server_ip_, ip, 15);
}
