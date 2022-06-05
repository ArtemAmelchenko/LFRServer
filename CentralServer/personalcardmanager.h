#ifndef PERSONALCARDMANAGER_H
#define PERSONALCARDMANAGER_H

#include <string>
#include <vector>

struct PersonalCard
{
    std::string id;
    std::string imagePath;
    std::string name, surname, lastname;
    std::string post, subdivision;
	int brightnessCorrection, contrastCorrection;
};

//класс менеджера персональных карт
class PersonalCardManager
{
    std::vector<PersonalCard> cards;

public:
    const std::vector<PersonalCard> *personalCards() const;

    void addCard(const PersonalCard &card);
    void editCard(const PersonalCard &card);
    void deleteCard(int cardIndex);
    void deleteCard(const std::string &cardID);
    void loadCards(const std::string &filename);
    void saveCards(const std::string &filename);

    void updateCards(const std::vector<PersonalCard> &newCards);

};

#endif // PERSONALCARDMANAGER_H
