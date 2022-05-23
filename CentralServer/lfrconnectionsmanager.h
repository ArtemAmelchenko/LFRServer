#ifndef LFRCONNECTIONSMANAGER_H
#define LFRCONNECTIONSMANAGER_H
#include <list>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "lfrconnection.h"
#include "personalcardmanager.h"
#include <QDateTime>

class LFRConnectionsManager
{
public:
	LFRConnectionsManager(boost::asio::ip::tcp::endpoint ep);
	~LFRConnectionsManager();

	void accepting();

	void run();

    void personalCardAdded(const PersonalCard &card, LFRConnection *connection);
    void personalCardEdited(const PersonalCard &card, LFRConnection *connection);
    void personalCardDeleted(const PersonalCard &card, LFRConnection *connection);

    void newPassingEvent(const PassingEvent &event);

    const QList<PersonalCard> *personalCards() const;

private:

    void acceptNewConnection(std::shared_ptr<LFRConnection> connection, const boost::system::error_code& error);

	boost::asio::io_context context;
	boost::asio::ip::tcp::acceptor acceptor;

    std::list<std::shared_ptr<LFRConnection>> connections;
	std::mutex connectionMutex;

    PersonalCardManager cardManager;
};

#endif // LFRCONNECTIONSMANAGER_H
