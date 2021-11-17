// This is a nutclient.dll library usage example. If you have your Linux UPS server (NUT Server) configured to accept
// remote connections, you can use this program to read the state of an UPS attached to the Linux machine from a Windows machine.
// Usage:
// upsinfo <host> <port>
// where host and port correspond to those of the UPS Server.
// "192.168.1.8", 3493
#include <iostream>
#include <string>
#include <memory>
#include "../nutclient.h"
using namespace nut;
using namespace std;

int main(int argc, char * argv[])
{
    if (argc != 3) {
        cout << "Usage: upsinfo <host> <port>" << endl;
        exit(-1);
    }
    auto host = string(argv[1]);
    auto port = stoi(argv[2]);
    try {
        std::unique_ptr<Client> client(new TcpClient(host, port));
        auto devices = client->getDeviceNames();
        for (const auto &device: devices) {
            cout << device << endl;
            if (!devices.empty()) {
                auto variables = client->getDeviceVariableNames(device);
                for (const auto &variable: variables) {
                    auto value = client->getDeviceVariableValue(device, variable);
                    cout << '\t' << variable << ": ";
                    for (const auto &v: value)
                        cout << v << '\t';
                    cout << endl;
                }
            }
        }
    } catch (const UnknownHostException &e) {
        cout << "Unknown host" << endl;
    } catch (const NutException & e) {
        cout << e.what() << endl;
    }
    exit(0);
}
