#pragma once
#include <Arduino.h>
#include <vector>

struct User {
    String id, firstname, middlename, lastname, email, passwordHash, salt, role, lastCreditActivity, createdAt, dailyDispenseDate;
    int credits = 0, dailyDispenses = 0;
    uint32_t lastCreditActivityAt = 0;
};

bool initUsers();
bool registerUser(const String& firstname, const String& middlename, const String& lastname,
                  const String& email, const String& password, String& error);
User* findUserByEmail(const String& email);
User* findUserById(const String& id);
const std::vector<User>& getUsers();
bool changeUserPassword(User& user, const String& currentPassword, const String& newPassword);
bool verifyUserPassword(const User& user, const String& password);
bool saveUsers();
bool addUserCredits(User& user, int amount);
bool setUserCredits(User& user, int amount);
bool expireInactiveCredits(User& user);
bool getDailyDispenseCount(const User& user, String& date, int& count);
bool currentCreditActivity(String& timestamp, uint32_t& epoch);

String createSession(const String& userId);
User* sessionUser(const String& token);
void destroySession(const String& token);

bool appendDispenseLog(const String& userId, const String& productId, int qty);
String dispenseLogJson();
