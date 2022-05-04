#include "lfrconnectionsmanager.h"
#include <memory>
#include <iostream>
#include "personalcardmanager.h"

using namespace std;

LFRConnectionsManager::LFRConnectionsManager(boost::asio::ip::tcp::endpoint ep)
	: acceptor(context, ep)
{
    cardManager.loadCards("Cards.json");
}

void LFRConnectionsManager::accepting()
{
    connections.push_back(std::shared_ptr<LFRConnection>(new LFRConnection(context, this)));
	acceptor.async_accept(connections.back()->socket, boost::bind<void>([&](std::shared_ptr<LFRConnection> connection, const boost::system::error_code& error)
	{
							  acceptNewConnection(connection, error);
							  return;
						  }, connections.back(), _1));
}

void LFRConnectionsManager::run()
{
    context.run();
}

void LFRConnectionsManager::personalCardAdded(const PersonalCard &card, LFRConnection *connection)
{

}

void LFRConnectionsManager::personalCardEdited(const PersonalCard &card, LFRConnection *connection)
{

}

void LFRConnectionsManager::personalCardDeleted(const PersonalCard &card, LFRConnection *connection)
{

}

void LFRConnectionsManager::connectionClosed(LFRConnection *connection)
{

}

void LFRConnectionsManager::newPassingEvent(const PassingEvent &event)
{
    string str;
    str += "Personal ID: ";
    str += event.id.toString().toStdString();
    if (event.passed)
        str += (event.enterance) ? ("Entered") : ("Exited");
    else
        str += (event.enterance) ? ("Tried enter") : ("Tried exit");
    str += "Time: ";
    str += event.time.toString("dd.MM.yyyy hh:mm:ss").toStdString();

    cout << str << endl;
}

QDateTime LFRConnectionsManager::lastCardsEventTime()
{
    return _lastCardsEventTime;
}

const QList<PersonalCard> *LFRConnectionsManager::personalCards() const
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
    connection->start();
    connections.push_back(std::shared_ptr<LFRConnection>(new LFRConnection(context, this)));
	acceptor.async_accept(connections.back()->socket, boost::bind<void>([&](std::shared_ptr<LFRConnection> connection, const boost::system::error_code& error)
	{
							  acceptNewConnection(connection, error);
							  return;
						  }, connections.back(), _1));
}
