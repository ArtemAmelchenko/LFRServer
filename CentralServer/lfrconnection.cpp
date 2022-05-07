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


LFRConnection::LFRConnection(io_context &context, LFRConnectionsManager *manager)
    : socket(context), manager(manager), in(&buffer)
{
    lastPing = QDateTime::currentDateTime();
}

void LFRConnection::sayHello()
{
    //    std::string msg = "Hello!";
    //    socket.async_write_some(buffer(msg.data(), msg.size()), [](const boost::system::error_code& error, std::size_t bytes_transferred)
    //    {
    //        if (error)
    //        {
    //            //log debug << BIG ERROR
    //            return;
    //        }
    //        //log info << bytes_transferred send
    //    });
}

void LFRConnection::start()
{
    running = true;

    t = thread([&]()
    {
        cout << "Connection started" << endl;

        fullSyncronisation(manager->personalCards());

        async_read_until(socket, buffer, '\n', [&](const boost::system::error_code& error, std::size_t bytes_transferred)
        {
            onRead(error, bytes_transferred);
        });

        while(running)
        {
            if (lastSyncTime.secsTo(QDateTime::currentDateTime()) >= 10)
            {
                //log -> disconnect
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        socket.close();
        manager->connectionClosed(this);
    });
}

bool LFRConnection::isRunning() const
{
    return running;
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
    boost::asio::async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> restartQuery writed;
        cout << "sended sunc" << endl;
    });
    //socket.write_some(buff);
    msg = QString::number(personalCards->size()).toStdString();
    msg += "\n";
    buff = const_buffer(msg.data(), msg.size());
    boost::asio::async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> restartQuery writed;
        cout << "sended sunc" << endl;
    });

    for (int i = 0; i < personalCards->size(); ++i)
    {
        cout << "Sending card " << i << endl;
        msg = personalCardToJSON(personalCards->at(i));
        msg += "!end!";
        buff = const_buffer(msg.data(), msg.size());
        boost::asio::async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
        {
            cout << "card writed" << endl;
        });
        //boost::asio::write(socket, buff);
    }
    cout << "End sync" << endl;
}

void LFRConnection::personalCardAdded(const PersonalCard &card)
{
    if (!syncronised)
        return;
    std::string msg = "personal card added\n";
    msg += personalCardToJSON(card);
    msg += "!end!";
    const_buffer buff(msg.data(), msg.size());
    async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> LFRConnection::personalCardAdded writed;
    });
}

void LFRConnection::personalCardEdited(const PersonalCard &card)
{
    if (!syncronised)
        return;
    std::string msg = "personal card edited\n";
    msg += personalCardToJSON(card);
    msg += "!end!";
    const_buffer buff(msg.data(), msg.size());
    async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> LFRConnection::personalCardEdited writed;
    });
}

void LFRConnection::personalCardDeleted(const PersonalCard &card)
{
    if (!syncronised)
        return;
    std::string msg = "personal card deleted\n";
    msg += personalCardToJSON(card);
    msg += "!end!";
    const_buffer buff(msg.data(), msg.size());
    async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> LFRConnection::personalCardDeleted writed;
    });
}

void LFRConnection::restartQuery()
{
    if (!syncronised)
        return;
    std::string msg = "restart\n";
    const_buffer buff(msg.data(), msg.size());
    async_write(socket, buff, [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        //log -> restartQuery writed;
    });
}

void LFRConnection::onRead(const boost::system::error_code &err, size_t read_bytes)
{
    if (err)
    {
        //log debug << BIG ERROR
        cout << "BIG ERROR" << endl;
        return;
    }

    string msg;
    getline(in, msg);

    cout << msg << endl;
    if (msg == "ping")
    {
        lastPing = QDateTime::currentDateTime();
    }
    else if (msg == "my id")
    {
        lastSyncTime = QDateTime::currentDateTime();
    }
    else if (msg == "new passing event")
    {
        PassingEvent pe;
        if (!recvPassingEvent(pe))
        {
            // disconnect
        }
        manager->newPassingEvent(pe);
    }
    else if (msg == "personal card added")
    {
        PersonalCard pc;
        if (!recvPersonalCard(pc))
        {
            // disconnect
            cout << "pipisa" << endl;
        }
        cout << "card readed" << endl;
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

    async_read_until(socket, buffer, '\n', [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        onRead(error, bytes_transferred);
    });
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
    boost::asio::streambuf buff;
    std::istream istr(&buff);
    bool recv = false;
    async_read_until(socket, buff, "!end!", [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        recv = true;
        std::string msg;
        istr >> msg;
        event = passingEventFromJSON(msg);
    });
    int mil = 0;
    while (!recv)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        mil++;
        if (mil >= millisec)
            return false;
    }
    return true;
}

bool LFRConnection::recvPersonalCard(PersonalCard &card, int millisec)
{
    bool recv = false;
    async_read_until(socket, buffer, "!end!", [&](const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        cout << "mama ya prochital" << endl;
        recv = true;
        std::string msg;
        in >> msg;
        card = personalCardFromJSON(msg);
    });
    int mil = 0;
    while (!recv)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        mil++;
        if (mil >= millisec)
            return false;
    }
    return true;
}
