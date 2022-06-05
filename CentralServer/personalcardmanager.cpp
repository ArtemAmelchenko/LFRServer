#define BOOST_LOG_DYN_LINK 1
#include "personalcardmanager.h"
#include <iostream>
#include <fstream>
#include "json.hpp"
#include <boost/log/trivial.hpp>

const std::vector<PersonalCard> *PersonalCardManager::personalCards() const
{
    return &cards;
}

void PersonalCardManager::addCard(const PersonalCard &card)
{
    auto it = std::find_if(cards.begin(), cards.end(), [&](const PersonalCard &card){ return card.id == card.id; });
    if (it == cards.end())
        cards.push_back(card);
}

void PersonalCardManager::editCard(const PersonalCard &card)
{
    auto it = std::find_if(cards.begin(), cards.end(), [&](const PersonalCard &card){ return card.id == card.id; });
	if (it == cards.end())
        cards.push_back(card);
    else
        *it = card;
}

void PersonalCardManager::deleteCard(int cardIndex)
{
	if (cards.size() <= cardIndex)
		return;
	cards.erase(cards.begin() + cardIndex);
}

void PersonalCardManager::deleteCard(const std::string &cardID)
{
    std::remove_if(cards.begin(), cards.end(), [&](const PersonalCard &card){ return card.id == cardID; });
}

void PersonalCardManager::loadCards(const std::string &filename)
{
    BOOST_LOG_TRIVIAL(debug) << "card manager start loading cards";
    cards.clear();

	PersonalCard card;

    nlohmann::json j;
    std::ifstream file(filename);
    file >> j;
    for (int i = 0; i < j.size(); ++i)
	{
        card.id = j[i]["id"];
        card.imagePath = j[i]["imagePath"];
        card.name = j[i]["name"];
        card.surname = j[i]["surname"];
        card.lastname = j[i]["lastname"];
        card.post = j[i]["post"];
        card.subdivision = j[i]["subdivision"];
        card.brightnessCorrection = j[i]["brightness"];
        card.contrastCorrection = j[i]["contrast"];
        cards.push_back(card);
	}
    BOOST_LOG_TRIVIAL(debug) << "card manager readed " << cards.size() << " cards";
}

void PersonalCardManager::saveCards(const std::string &filename)
{
    nlohmann::json j = nlohmann::json::array();
    for (int i = 0; i < cards.size(); ++i)
	{
        j[i]["id"] = cards[i].id;
        j[i]["imagePath"] = cards[i].imagePath;
        j[i]["name"] = cards[i].name;
        j[i]["surname"] = cards[i].surname;
        j[i]["lastname"] = cards[i].lastname;
        j[i]["post"] = cards[i].post;
        j[i]["subdivision"] = cards[i].subdivision;
        j[i]["brightness"] = cards[i].brightnessCorrection;
        j[i]["contrast"] = cards[i].contrastCorrection;
	}
    std::ofstream file(filename);
    file << j;
}

void PersonalCardManager::updateCards(const std::vector<PersonalCard> &newCards)
{
	cards.clear();
    cards.insert(cards.begin(), newCards.begin(), newCards.end());
}
