#include "lfrconnection.h"
#include "lfrconnectionsmanager.h"
#include <chrono>
#include <thread>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>
#include <iostream>

using namespace boost::asio;
using namespace std;


LFRConnection::LFRConnection(LFRConnectionsManager *manager)
    : socket(context), in(&buffer), manager(manager)
{
}

void LFRConnection::start()
{
    running = true;

    lastPing = QDateTime::currentDateTime();
    cout << "Connection started" << endl;

    t = thread([&]()
    {

        fullSyncronisation(manager->personalCards());

        thread workThread = thread([&]()
        {
            string msg;
            boost::system::error_code ec;

            while(running)
            {
                boost::asio::read_until(socket, buffer, '\n', ec);
                if (ec)
                {
                    cout << ec.what() << endl;
                    running = false;
                    break;
                }
                lastPing = QDateTime::currentDateTime();
                getline(in, msg);

                cout << msg << endl;
                if (msg == "my id")
                {
                    boost::asio::read_until(socket, buffer, '\n');
                    string id;
                    getline(in, id);
                    userID = QUuid(id.c_str());
                }
                else if (msg == "new passing event")
                {
                    PassingEvent pe;
                    if (!recvPassingEvent(pe))
                    {
                        cout << "ZALUPA" << endl;
                        // disconnect
                    }
                    manager->newPassingEvent(pe);
                }
                else if (msg == "personal card added")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        cout << "ZALUPA" << endl;
                        // disconnect
                    }
                    manager->personalCardAdded(pc, this);
                }
                else if (msg == "personal card edited")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        // disconnect
                    }
                    manager->personalCardEdited(pc, this);
                }
                else if (msg == "personal card deleted")
                {
                    PersonalCard pc;
                    if (!recvPersonalCard(pc))
                    {
                        // disconnect
                    }
                    manager->personalCardDeleted(pc, this);
                }
            }
        });

        sending();
        running = false;
        workThread.join();
        socket.close();
        manager->connectionClosed(this);
    });
    t.detach();
}

void LFRConnection::stop()
{
    running = false;
}

void LFRConnection::fullSyncronisation(const QList<PersonalCard> *personalCards)
{
    cout << "Full sync" << endl;
    std::string msg = "sync\n";
    const_buffer buff(msg.data(), msg.size());
    boost::asio::write(socket, buff);
    msg = QString::number(personalCards->size()).toStdString();
    msg += "\n";
    buff = const_buffer(msg.data(), msg.size());
    boost::asio::write(socket, buff);

    for (int i = 0; i < personalCards->size(); ++i)
    {
        cout << "Sending card " << i << endl;
        msg = personalCardToJSON(personalCards->at(i));
        msg += "!end!";
        buff = const_buffer(msg.data(), msg.size());
        boost::asio::write(socket, buff);
    }
    cout << "End sync" << endl;
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
    std::string msg = "restart\n";
    const_buffer buff(msg.data(), msg.size());
    async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> restartQuery writed;
    });
}

void LFRConnection::sending()
{
    while(running)
    {
        if (lastPing.secsTo(QDateTime::currentDateTime()) >= 10)
        {
            cout << "Last ping > 10" << endl;
            //log -> disconnect
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
                //log -> LFRConnection::personalCardAdded writed;
                break;
            case 2:     //edited
                msg = "personal card edited\n";
                msg += personalCardToJSON(q.card);
                msg += "!end!";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                //log -> writed;
                break;
            case 3:     //deleted
                msg = "personal card deleted\n";
                msg += personalCardToJSON(q.card);
                msg += "!end!";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                //log -> writed;
                break;
            case 4:     //deleted
                msg = "restart\n";
                buff = const_buffer(msg.data(), msg.size());
                write(socket, buff);
                //log -> restart;
                break;
            default:
                //unknown type
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

string LFRConnection::personalCardToJSON(const PersonalCard &card)
{
    QJsonDocument doc;
    QJsonObject obj;
    obj.insert("id", card.id.toString());
    obj.insert("imagePath", card.imagePath);
    obj.insert("lastname", card.lastname);
    obj.insert("name", card.name);
    obj.insert("post", card.post);
    obj.insert("subdivision", card.subdivision);
    obj.insert("surname", card.surname);
    obj.insert("brightness", card.brightnessCorrection);
    obj.insert("contrast", card.contrastCorrection);
    doc.setObject(obj);

    QByteArray b = doc.toJson();
    string s(b.data());

    return s;
}

PersonalCard LFRConnection::personalCardFromJSON(const string &str)
{
    PersonalCard card;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(str));
    QJsonObject obj = doc.object();
    card.id = QUuid::fromString(obj.take("id").toString());
    card.imagePath = obj.take("imagepath").toString();
    card.lastname = obj.take("lastname").toString();
    card.name = obj.take("name").toString();
    card.post = obj.take("post").toString();
    card.subdivision = obj.take("subdivision").toString();
    card.surname = obj.take("surname").toString();
    card.brightnessCorrection = obj.take("brightness").toInt();
    card.contrastCorrection = obj.take("contrast").toInt();
    return card;
}

string LFRConnection::passingEventToJSON(const PassingEvent &event)
{
    QJsonDocument doc;
    QJsonObject obj;
    obj.insert("id", event.id.toString());
    obj.insert("enterance", event.enterance);
    obj.insert("passed", event.passed);
    obj.insert("time", event.time.toString("yyyy.MM.dd.hh.mm.ss"));
    doc.setObject(obj);
    return doc.toJson().toStdString();
}

PassingEvent LFRConnection::passingEventFromJSON(const string &str)
{
    PassingEvent event;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(str));
    QJsonObject obj = doc.object();
    event.id = QUuid::fromString(obj.take("id").toString());
    event.enterance = obj.take("enterance").toBool();
    event.passed = obj.take("passed").toBool();
    event.time = QDateTime::fromString(obj.take("time").toString(), "yyyy.MM.dd.hh.mm.ss");
    return event;
}

bool LFRConnection::recvPassingEvent(PassingEvent &event, int millisec)
{
    atomic<bool> recv = false;
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
    atomic<bool> recv = false;
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
