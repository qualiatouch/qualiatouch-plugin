#include "DmxRegistry.hpp"

DmxRegistry::DmxRegistry() {
    cout << "DmxRegistry()" << endl;
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

    if (!ola_client->SendDmx(dmxUniverse, buffer)) {
        cerr << "Sending DMX blackout failed" << endl;
    }
}

DmxRegistry& DmxRegistry::instance() {
    static DmxRegistry registry;
    return registry;
}

void DmxRegistry::registerModule(DmxOut1* module) {
    cout << "addModule " << module->getId() << endl;
    modules.push_back(module);
    cout << "size is now " << modules.size() << endl;
}

void DmxRegistry::unregisterModule(DmxOut1* module) {
    cout << "removeModule " << module->getId() << endl;
    modules.erase(
        std::remove(modules.begin(), modules.end(), module),
        modules.end()
    );
    cout << "size is now " << modules.size() << endl;
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

void DmxRegistry::sendDmx() {
    int nbModules = modules.size();
    for (int i = 0; i < nbModules; i++) {
        DmxOut1* m = modules.at(i);
        if (m->blackoutTriggered) {
            if (debug) {
                cout << "BLACKOUT triggered on module " << i << " - sending blackout" << endl;
            }
            buffer.Blackout();
            break;
        }

        if (debug) {
            cout << "   setting channel " << m->dmxChannel << " to DMX value " << m->dmxValue << endl;
        }
        buffer.SetChannel(m->dmxChannel, m->dmxValue);
    }

    if (debug) {
        cout << "sending DMX : " << buffer.ToString();
    }

    if (!ola_client->SendDmx(dmxUniverse, buffer)) {
        if (debug) {
            cout << "Sending DMX failed" << endl;
        }
    } else {
        if (debug) {
            cout << "sent" << endl;
        }
    }
}
