#include "DmxRegistry.hpp"

DmxRegistry::DmxRegistry() {
}

DmxRegistry& DmxRegistry::instance() {
    static DmxRegistry registry;
    return registry;
}

void DmxRegistry::initOla() {
    if (debug) {
        cout << "OLA : init logging" << endl;
    }
    ola::InitLogging(ola::OLA_LOG_WARN, ola::OLA_LOG_STDERR);

    if (debug) {
        cout << "OLA : init client" << endl;
    }
    ola_client = std::unique_ptr<ola::client::StreamingClient>(new ola::client::StreamingClient());

    if (debug) {
        cout << "OLA : setup client" << endl;
    }
    if (!ola_client->Setup()) {
        std::cerr << "Setup failed" << endl;
    }

    if (debug) {
        cout << "OLA : blackout" << endl;
    }

    buffer.Blackout();
    if (!ola_client->SendDmx(dmxUniverse, buffer)) {
        cerr << "Sending DMX blackout failed" << endl;
    }
}

void DmxRegistry::registerModule(AbstractDmxModule* module) {
    if (ola_client == nullptr) {
        initOla();
    }
    modules.push_back(module);
}

void DmxRegistry::unregisterModule(AbstractDmxModule* module) {
    modules.erase(
        std::remove(modules.begin(), modules.end(), module),
        modules.end()
    );
    if (modules.size() == 0) {
        if (debug) {
            cout << "No DMX module" << endl;
            cout << "OLA : blackout" << endl;
        }

        buffer.Blackout();
        if (!ola_client->SendDmx(dmxUniverse, buffer)) {
            cerr << "Sending DMX blackout failed" << endl;
        }

        if (debug) {
            cout << "stopping OLA client" << endl;
        }
        ola_client->Stop();
        ola_client = nullptr;
    }
}

bool DmxRegistry::isMaster(int64_t id) {
    return id == modules.front()->getId();
}

void DmxRegistry::trigger(int64_t id) {
    if (!isMaster(id)) {
        return;
    }

    sendDmx();
}

int DmxRegistry::getDmxUniverse() {
    return dmxUniverse;
}

void DmxRegistry::setDmxUniverse(int universe) {
    dmxUniverse = universe;
}

void DmxRegistry::sendDmx() {
    int nbModules = modules.size();
    for (int i = 0; i < nbModules; i++) {
        AbstractDmxModule* m = modules.at(i);
        if (m->blackoutTriggered) {
            if (debug) {
                cout << "BLACKOUT triggered on module " << i << " - sending blackout" << endl;
            }
            buffer.Blackout();
            break;
        }

        for (const std::pair<unsigned int, uint8_t>& pair : m->channelsValues) {
            if (debug) {
                std::cout << "  channel " << pair.first << " value " << (int) pair.second << std::endl;
            }
            buffer.SetChannel(pair.first, pair.second);
        }
    }

    if (debug) {
        cout << "sending DMX to universe " << dmxUniverse << " : " << buffer.ToString();
    }

    if (!ola_client->SendDmx(dmxUniverse, buffer)) {
        if (debug) {
            cout << "Sending DMX failed" << endl;
        }
    } else {
        if (debug) {
            cout << " - sent" << endl;
        }
    }
}
