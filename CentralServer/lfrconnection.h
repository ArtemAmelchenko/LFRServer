#ifndef LFRCONNECTION_H
#define LFRCONNECTION_H
#include <queue>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include "personalcardmanager.h"

struct PassingEvent
{
    std::string id;
	bool enterance;
	bool passed;
    std::string time;
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

    void fullSyncronisation(const std::vector<PersonalCard> *personalCards);

	void personalCardAdded(const PersonalCard &card);
	void personalCardEdited(const PersonalCard &card);
	void personalCardDeleted(const PersonalCard &card);

	void restartQuery();

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
    bool recvImage(int millisec = 3000);
    void writeImage(const std::string &data, const std::string &fileName);

    void sendImage();

	bool sendRestartQuery();

	std::string userName;	//user information
    std::string userID;

    boost::asio::io_context context;
    boost::asio::ip::tcp::socket socket;
	bool running;
    bool end;

    boost::asio::streambuf buffer;
    std::istream in;

    std::shared_ptr<LFRConnectionsManager> manager;

    std::chrono::_V2::system_clock::time_point lastPing;

    std::thread t;

    std::queue<Query> queries;
};

#endif // LFRCONNECTION_H
