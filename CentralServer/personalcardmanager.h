#ifndef PERSONALCARDMANAGER_H
#define PERSONALCARDMANAGER_H

#include <QObject>
#include <QFile>
#include <QUuid>
#include <QDateTime>

struct PersonalCard
{
	QUuid id;
	QString imagePath;
	QString name, surname, lastname;
	QString post, subdivision;
	int brightnessCorrection, contrastCorrection;
};

//класс менеджера персональных карт
class PersonalCardManager
{
	QList<PersonalCard> cards;

public:
	const QList<PersonalCard> *personalCards() const;

    void addCard(const PersonalCard &card);
    void editCard(const PersonalCard &card);
    void deleteCard(int cardIndex);
    void deleteCard(const QUuid &cardID);
	void loadCards(const QString &filename);
	void saveCards(const QString &filename);

	void updateCards(const QList<PersonalCard> &newCards);

};

#endif // PERSONALCARDMANAGER_H
