#define BOOST_LOG_DYN_LINK 1
#include "lfrconnectionsmanager.h"
#include <memory>
#include <iostream>
#include "personalcardmanager.h"
#include <boost/log/trivial.hpp>

using namespace std;

LFRConnectionsManager::LFRConnectionsManager(boost::asio::ip::tcp::endpoint ep)
    : acceptor(context, ep)
{
    cardManager.loadCards("Cards.json");
}

void LFRConnectionsManager::accepting()
{
    BOOST_LOG_TRIVIAL(debug) << "manager: accepting";
    connections.push_back(std::shared_ptr<LFRConnection>(new LFRConnection(this)));
    acceptor.async_accept(connections.back()->socket, boost::bind<void>([&](std::shared_ptr<LFRConnection> connection, const boost::system::error_code& error)
                          {
                              acceptNewConnection(connection, error);
                              accepting();
                          }, connections.back(), _1));
}

void LFRConnectionsManager::run()
{
//    thread cleaningThread = thread([&]()
//    {
//        while(true)
//        {
//            BOOST_LOG_TRIVIAL(debug) << "manager: cleaning closed connections";
//            if (connections.front()->end)
//                connections.remove(connections.front());
//            //remove_if(connections.begin(), connections.end(), [&](const shared_ptr<LFRConnection> &c) {return c->isEnd();});
//            this_thread::sleep_for(chrono::milliseconds(1000));
//        }
//    });
    context.run();
}

void LFRConnectionsManager::personalCardAdded(const PersonalCard &card, LFRConnection *connection)
{
    BOOST_LOG_TRIVIAL(debug) << "manager: personal card added";
    cardManager.addCard(card);
    for (auto &t : connections)
        if (t.get() != connection)
            t->personalCardAdded(card);
}

void LFRConnectionsManager::personalCardEdited(const PersonalCard &card, LFRConnection *connection)
{
    BOOST_LOG_TRIVIAL(debug) << "manager: personal card edited";
    cardManager.editCard(card);
    for (auto &t : connections)
        if (t.get() != connection)
            t->personalCardEdited(card);
}

void LFRConnectionsManager::personalCardDeleted(const PersonalCard &card, LFRConnection *connection)
{
    BOOST_LOG_TRIVIAL(debug) << "manager: personal card deleted";
    cardManager.deleteCard(card.id);
    for (auto &t : connections)
        if (t.get() != connection)
            t->personalCardDeleted(card);
}

void LFRConnectionsManager::newPassingEvent(const PassingEvent &event)
{
    string str;
    str += "Personal ID: ";
    str += event.id + " ";
    if (event.passed)
        str += (event.enterance) ? ("Entered") : ("Exited");
    else
        str += (event.enterance) ? ("Tried enter") : ("Tried exit");
    str += " ";
    str += "Time: ";
    str += event.time;

    BOOST_LOG_TRIVIAL(debug) << "manager: new passing event: " << str;
}

const std::vector<PersonalCard> *LFRConnectionsManager::personalCards() const
{
    return cardManager.personalCards();
}

void LFRConnectionsManager::acceptNewConnection(std::shared_ptr<LFRConnection> connection, const boost::system::error_code &error)
{
    if (error)
    {
        //BIG ERROR
        return;
    }
    //New connection accepted
    BOOST_LOG_TRIVIAL(debug) << "manager: new connection";
    connection->start();
}
