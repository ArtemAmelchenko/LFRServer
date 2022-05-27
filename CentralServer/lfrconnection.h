#ifndef LFRCONNECTION_H
#define LFRCONNECTION_H
#include <QUuid>
#include <QDateTime>
#include <QJsonDocument>
#include <queue>
#include <thread>
#include <boost/asio.hpp>
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
    int type; //1 - added, 2 - edited, 3 - deleted, 4 - restart
    PersonalCard card;
};

class LFRConnectionsManager;

class LFRConnection
{
public:
    LFRConnection(LFRConnectionsManager *manager);

	void start();

	void stop();

    void fullSyncronisation(const QList<PersonalCard> *personalCards);

	void personalCardAdded(const PersonalCard &card);
	void personalCardEdited(const PersonalCard &card);
	void personalCardDeleted(const PersonalCard &card);

	void restartQuery();

	bool getImage(const std::string &fileName);

    bool isEnd() const;

	friend class LFRConnectionsManager;

private:

    void sending();

    std::string personalCardToJSON(const PersonalCard &card);
    PersonalCard personalCardFromJSON(const std::string &str);
    std::string passingEventToJSON(const PassingEvent &event);
    PassingEvent passingEventFromJSON(const std::string &str);

    bool recvPassingEvent(PassingEvent &event, int millisec = 3000);
    bool recvPersonalCard(PersonalCard &card, int millisec = 3000);
	bool recvImage(const std::string &fileName, int millisec = 3000);
    void writeImage(const std::string &data, const std::string &fileName);

    void sendImage();

	bool sendRestartQuery();

	std::string userName;	//user information
    QUuid userID;

    boost::asio::io_context context;
    boost::asio::ip::tcp::socket socket;
	bool running;
    bool end;

    boost::asio::streambuf buffer;
    std::istream in;

    std::shared_ptr<LFRConnectionsManager> manager;

    QDateTime lastPing;

    std::thread t;

    std::queue<Query> queries;
};

#endif // LFRCONNECTION_H
