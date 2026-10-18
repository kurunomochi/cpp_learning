#pragma once
#include <string>
namespace httplib { class Server; }
namespace task {
class TaskRepository;
void register_routes(httplib::Server& server, TaskRepository& repository, std::string api_token);
}
