//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include "asgard/driver.hpp"

namespace {

// Configuration (this should be in a configuration file)
const char* server_socket_path = "/tmp/asgard_socket";
const char* client_socket_path = "/tmp/asgard_wol_socket";
const std::size_t delay_ms = 5000;

asgard::driver_connector driver;

// The remote IDs
int source_id = -1;
int action_id = -1;

void stop(){
    std::cout << "asgard:system: stop the driver" << std::endl;

    asgard::unregister_action(driver, source_id, action_id);
    asgard::unregister_source(driver, source_id);

    // Unlink the client socket
    unlink(client_socket_path);

    // Close the socket
    close(driver.socket_fd);
}

void terminate(int){
    stop();

    std::exit(0);
}

} //End of anonymous namespace

int main(){
    // Open the connection
    if(!asgard::open_driver_connection(driver, client_socket_path, server_socket_path)){
        return 1;
    }

    // Register signals for "proper" shutdown
    signal(SIGTERM, terminate);
    signal(SIGINT, terminate);

    // Register the source and actions
    source_id = asgard::register_source(driver, "wol");
    action_id = asgard::register_action(driver, source_id, "SIMPLE", "windows"); //TODO This action will be removed

    // Listen for messages from the server
    while(true){
        socklen_t address_length              = sizeof(struct sockaddr_un);
        auto bytes_received                   = recvfrom(driver.socket_fd, driver.receive_buffer, asgard::buffer_size, 0, (struct sockaddr*)&driver.server_address, &address_length);
        driver.receive_buffer[bytes_received] = '\0';

        std::string message(driver.receive_buffer);
        std::stringstream message_ss(message);

        std::string command;
        message_ss >> command;

        if(command == "ACTION"){
            std::string action;
            message_ss >> action;

            if(action == "windows"){
                auto result = asgard::exec_command("wakeonlan BC:EE:7B:87:9D:61");

                if(result.first){
                    std::cout << "wakeonlan failed(" << result.first << ") with result: \n" << result.second << std::endl;
                }
            } else if(action == "wake"){
                //TODO Future generic action with parameter
            }
        }
    }

    stop();

    return 0;
}
