#ifndef LFRCONNECTION_H
#define LFRCONNECTION_H
#include <boost/asio.hpp>
#include <QUuid>
#include <QDateTime>
#include <QJsonDocument>
#include <queue>
#include <thread>
#include "personalcardmanager.h"

struct PassingEvent
{
    QUuid id;
	bool enterance;
	bool passed;
	QDateTime time;
};

struct Query
{
    int type; //1 - added, 2 - edited, 3 - deleted
    PersonalCard card;
};

class LFRConnectionsManager;

class LFRConnection
{
public:
    LFRConnection(boost::asio::io_context &context, LFRConnectionsManager *manager);

	void sayHello();

	void start();

	bool isRunning() const;

	void stop();

    void fullSyncronisation(const QList<PersonalCard> *personalCards);

	void personalCardAdded(const PersonalCard &card);
	void personalCardEdited(const PersonalCard &card);
	void personalCardDeleted(const PersonalCard &card);

	void restartQuery();

	friend class LFRConnectionsManager;

private:

    void onRead(const boost::system::error_code &err, size_t read_bytes);

    std::string personalCardToJSON(const PersonalCard &card);
    PersonalCard personalCardFromJSON(const std::string &str);
    std::string passingEventToJSON(const PassingEvent &event);
    PassingEvent passingEventFromJSON(const std::string &str);

    bool recvPassingEvent(PassingEvent &event, int millisec = 3000);
    bool recvPersonalCard(PersonalCard &card, int millisec = 3000);

	bool sendRestartQuery();

	std::string userName;	//user information

	boost::asio::ip::tcp::socket socket;
	bool running;
    bool syncronised;

    boost::asio::streambuf buffer;
    std::istream in;

	std::shared_ptr<LFRConnectionsManager> manager;

    QDateTime lastSyncTime;
    QDateTime lastPing;

    std::thread t;
};

#endif // LFRCONNECTION_H
