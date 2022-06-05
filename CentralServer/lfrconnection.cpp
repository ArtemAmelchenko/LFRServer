#define BOOST_LOG_DYN_LINK 1
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include "json.hpp"
#include <boost/log/trivial.hpp>
#include "lfrconnection.h"
#include "lfrconnectionsmanager.h"

using namespace boost::asio;
using namespace std;


LFRConnection::LFRConnection(LFRConnectionsManager *manager)
    : socket(context), end(false), in(&buffer), manager(manager)
{
}

void LFRConnection::start()
{
    running = true;

    lastPing = std::chrono::system_clock::now();
    BOOST_LOG_TRIVIAL(debug) << "connection: started";

    t = thread([&]()
    {

        fullSyncronisation(manager->personalCards());

        thread workThread = thread([&]()
        {
            string msg;
            boost::system::error_code ec;

            while(running)
            {
                BOOST_LOG_TRIVIAL(debug) << "connection: reading socket";
                boost::asio::read_until(socket, buffer, '\n', ec);
                if (ec)
                {
                    BOOST_LOG_TRIVIAL(debug) << "connection: read until failed, closing connection";
                    running = false;
                    break;
                }
                lastPing = std::chrono::system_clock::now();
                getline(in, msg);

                BOOST_LOG_TRIVIAL(debug) << "connection: query - " << msg;
                if (msg == "my id")
                {
                    boost::asio::read_until(socket, buffer, '\n');
                    string id;
                    getline(in, id);
                    userID = id;
                }
                else if (msg == "new passing event")
                {
                    PassingEvent pe;
                    if (!recvPassingEvent(pe))
                    {
                        BOOST_LOG_TRIVIAL(debug) << "connection: failed recieve passing event";
                        running = false;
                        break;
                    }
                    manager->newPassingEvent(pe);
                }
                else if (msg == "personal card added")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        BOOST_LOG_TRIVIAL(debug) << "connection: failed recieve personal card";
                        running = false;
                        break;
                    }
                    manager->personalCardAdded(pc, this);
                }
                else if (msg == "personal card edited")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        BOOST_LOG_TRIVIAL(debug) << "connection: failed recieve personal card";
                        running = false;
                        break;
                    }
                    manager->personalCardEdited(pc, this);
                }
                else if (msg == "personal card deleted")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        BOOST_LOG_TRIVIAL(debug) << "connection: failed recieve personal card";
                        running = false;
                        break;
                    }
                    manager->personalCardDeleted(pc, this);
                }
                else if (msg == "personal card deleted")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        BOOST_LOG_TRIVIAL(debug) << "connection: failed recieve personal card";
                        running = false;
                        break;
                    }
                    manager->personalCardDeleted(pc, this);
                }
                else if (msg == "image added")
                {
                    BOOST_LOG_TRIVIAL(debug) << "connection: recieving image";
                    if (!recvImage())
                    {
                        BOOST_LOG_TRIVIAL(debug) << "connection: failed recieve image";
                        running = false;
                        break;
                    }
                    BOOST_LOG_TRIVIAL(debug) << "connection: image recieved";
                }
                else if (msg == "get image")
                {
                    BOOST_LOG_TRIVIAL(debug) << "connection: sending image";
                    std::string msg = "sending image\n";
                    boost::asio::write(socket, const_buffer(msg.data(), msg.size()));
                    sendImage();
                    BOOST_LOG_TRIVIAL(debug) << "connection: image sended";
                }
            }
        });

        sending();
        running = false;
        BOOST_LOG_TRIVIAL(debug) << "connection: waiting workThread stop";
        workThread.join();
        BOOST_LOG_TRIVIAL(debug) << "connection: closing socket";
        socket.close();
        end = true;
    });
    t.detach();
}

void LFRConnection::stop()
{
    BOOST_LOG_TRIVIAL(debug) << "connection: stopped";
    running = false;
}

void LFRConnection::fullSyncronisation(const std::vector<PersonalCard> *personalCards)
{
    BOOST_LOG_TRIVIAL(debug) << "connection: full sync";
    std::string msg = "sync\n";
    boost::asio::write(socket, const_buffer(msg.data(), msg.size()));
    msg = to_string(personalCards->size()) + "\n";
    boost::asio::write(socket, const_buffer(msg.data(), msg.size()));

    for (int i = 0; i < personalCards->size(); ++i)
    {
        msg = personalCardToJSON(personalCards->at(i)) + "!end!";
        boost::asio::write(socket, const_buffer(msg.data(), msg.size()));
    }
    BOOST_LOG_TRIVIAL(debug) << "connection: sync end, send " << personalCards->size() << " cards";
}

void LFRConnection::personalCardAdded(const PersonalCard &card)
{
    Query q;
    q.type = 1;
    q.card = card;
    queries.push(q);
}

void LFRConnection::personalCardEdited(const PersonalCard &card)
{
    Query q;
    q.type = 2;
    q.card = card;
    queries.push(q);
}

void LFRConnection::personalCardDeleted(const PersonalCard &card)
{
    Query q;
    q.type = 3;
    q.card = card;
    queries.push(q);
}

void LFRConnection::restartQuery()
{
    Query q;
    q.type = 4;
    queries.push(q);
}

bool LFRConnection::isEnd() const
{
    return end;
}

void LFRConnection::sending()
{
    while(running)
    {
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastPing).count() >= 10)
        {
            BOOST_LOG_TRIVIAL(debug) << "connection: no msg for 10 sec -> disconnect";
            running = false;
            break;
        }
        std::string msg;
        const_buffer buff;
        while(!queries.empty())
        {
            Query q = queries.front();
            queries.pop();
            switch (q.type)
            {
            case 1:     //added
                msg = "personal card added\n";
                msg += personalCardToJSON(q.card);
                msg += "!end!";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                BOOST_LOG_TRIVIAL(debug) << "connection: send personal card added";
                break;
            case 2:     //edited
                msg = "personal card edited\n";
                msg += personalCardToJSON(q.card);
                msg += "!end!";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                BOOST_LOG_TRIVIAL(debug) << "connection: send personal card edited";
                break;
            case 3:     //deleted
                msg = "personal card deleted\n";
                msg += personalCardToJSON(q.card);
                msg += "!end!";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                BOOST_LOG_TRIVIAL(debug) << "connection: send personal card deleted";
                break;
            case 4:     //restart
                msg = "restart\n";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                BOOST_LOG_TRIVIAL(debug) << "connection: send restart msg";
                break;
            default:
                BOOST_LOG_TRIVIAL(debug) << "connection: unknown query type";
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

string LFRConnection::personalCardToJSON(const PersonalCard &card)
{
    nlohmann::json j;
    j["id"] = card.id;
    j["imagePath"] = card.imagePath;
    j["lastname"] = card.lastname;
    j["name"] = card.name;
    j["post"] = card.post;
    j["subdivision"] = card.subdivision;
    j["surname"] = card.surname;
    j["brightness"] = card.brightnessCorrection;
    j["contrast"] = card.contrastCorrection;

    return j.dump();
}

PersonalCard LFRConnection::personalCardFromJSON(const string &str)
{
    nlohmann::json j;
    j.parse(str);
    PersonalCard card;
    card.id = j["id"];
    card.imagePath = j["imagePath"];
    card.lastname = j["lastname"];
    card.name = j["name"];
    card.post = j["post"];
    card.subdivision = j["subdivision"];
    card.surname = j["surname"];
    card.brightnessCorrection = j["brightness"];
    card.contrastCorrection = j["contrast"];
    return card;
}

string LFRConnection::passingEventToJSON(const PassingEvent &event)
{
    nlohmann::json j;
    j["id"] = event.id;
    j["enterance"] = event.enterance;
    j["passed"] = event.passed;
    j["time"] = event.time;
    return j.dump();
}

PassingEvent LFRConnection::passingEventFromJSON(const string &str)
{
    nlohmann::json j;
    j.parse(str);
    PassingEvent event;
    event.id = j["id"];
    event.enterance = j["enterance"];
    event.passed = j["passed"];
    event.time = j["time"];
    return event;
}

bool LFRConnection::recvPassingEvent(PassingEvent &event, int millisec)
{
    atomic<bool> recv(false);
    async_read_until(socket, buffer, "!end!", [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        recv = true;
        std::string msg(std::istreambuf_iterator<char>(in), {});
        msg = msg.substr(0, msg.size() - 5);
        event = passingEventFromJSON(msg);
    });
    context.reset();
    context.run_for(std::chrono::milliseconds(millisec));
    return recv;
}

bool LFRConnection::recvPersonalCard(PersonalCard &card, int millisec)
{
    atomic<bool> recv(false);
    async_read_until(socket, buffer, "!end!", [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        recv = true;
        std::string msg(std::istreambuf_iterator<char>(in), {});
        msg = msg.substr(0, msg.size() - 5);
        card = personalCardFromJSON(msg);
    });
    context.reset();
    context.run_for(std::chrono::milliseconds(millisec));
    return recv;
}

bool LFRConnection::recvImage(int millisec)
{
    atomic<bool> recv(false);
    int length;
    string fileName;
    read_until(socket, buffer, "\n");
    getline(in, fileName);
    BOOST_LOG_TRIVIAL(debug) << "connection: recieving " << fileName;
    read_until(socket, buffer, "\n");
    string len;
    getline(in, len);
    length = atoi(len.c_str());
    BOOST_LOG_TRIVIAL(debug) << "connection: length " << length;
    string data;
    BOOST_LOG_TRIVIAL(debug) << "connection: buff length " << buffer.size();
    //BOOST_LOG_TRIVIAL(debug) << "connection: buff : " << in.rdbuf();
    async_read(socket, buffer, transfer_exactly(length - buffer.size()), [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        BOOST_LOG_TRIVIAL(debug) << "connection: recv length " << bytes_transferred;
        recv = true;
        std::string msg(std::istreambuf_iterator<char>(in), {});
        BOOST_LOG_TRIVIAL(debug) << "connection: msg length " << msg.size();
        writeImage(msg, fileName);
    });
    context.reset();
    context.run_for(std::chrono::milliseconds(millisec));
    return recv;
}

void LFRConnection::writeImage(const string &data, const string &fileName)
{
    ofstream f("./img/" + fileName);
    f.write(data.c_str(), data.size());
    f.close();
}

void LFRConnection::sendImage()
{
    string fileName;
    read_until(socket, buffer, "\n");
    getline(in, fileName);

    BOOST_LOG_TRIVIAL(debug) << "connection: sending " << fileName;

    ifstream f("./img/" + fileName);
    f.seekg(0, std::ios::end);
    size_t size = f.tellg();
    std::string data(size, ' ');
    f.seekg(0);
    f.read(&data[0], size);
    f.close();

    int length = data.size();
    string strLen = to_string(length);
    BOOST_LOG_TRIVIAL(debug) << "connection: sending length " << strLen;
    write(socket, const_buffer(strLen.data(), strLen.size()));
    write(socket, const_buffer(data.data(), data.size()));
}
