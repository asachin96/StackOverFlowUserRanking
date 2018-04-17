#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <stdlib.h> /* atoi */
#include <sys/time.h>
#include <unistd.h>
#include <ctime> /* tm */
#include <chrono> /* time_point */
#include <queue>
#include "Utils.hpp"

static const int PAST_MONTHS = 6;
static const int TOP_USERS = 10;

// Represents an entry in posts.xml
// Some fields omitted
struct Post {
    enum PostType {
        QUESTION,
        ANSWER,
        OTHER
    };

    std::string id;
    PostType postTypeId;
    std::string ownerUserId;
    std::tm creationDate;
    int score;

    Post(std::string id, std::string postTypeIdStr, std::string ownerUserId, struct std::tm creationDate, int score)
    : id(id), ownerUserId(ownerUserId), creationDate(creationDate), score(score) {
        if (postTypeIdStr == "1") {
            postTypeId = QUESTION;
        } else if (postTypeIdStr == "2") {
            postTypeId = ANSWER;
        } else {
            postTypeId = OTHER;
        }
    }
};

void readUsers(const std::string &filename, std::unordered_map<std::string, std::string> &users) {
    std::ifstream fin;
    fin.open(filename.c_str());
    std::string line;
    while (std::getline(fin, line)) {
        std::string id = parseFieldFromLine(line, "Id");
        std::string displayName = parseFieldFromLine(line, "DisplayName");
        if (!id.empty() && !displayName.empty()) {
            users[id] = displayName;
        }
    }
}

void readPosts(const std::string &filename,  std::vector<Post> &posts, std::chrono::system_clock::time_point minValidDate) {
    std::ifstream fin;
    fin.open(filename.c_str());
    std::string line;
    while (std::getline(fin, line)) {
        std::string id = parseFieldFromLine(line, "Id");
        std::string postTypeId = parseFieldFromLine(line, "PostTypeId");
        std::string ownerUserId = parseFieldFromLine(line, "OwnerUserId");
        int score = parseIntFromLine(line, "Score");
        std::string creationDate = parseFieldFromLine(line, "CreationDate");
        struct std::tm creationDateTm;
        strptime(creationDate.c_str(), "%Y-%m-%dT%H:%M:%S.%f", &creationDateTm);
        if (!id.empty() && !ownerUserId.empty() && !postTypeId.empty() && !creationDate.empty() && isValidDate(creationDateTm, minValidDate)) {
            Post p(id, postTypeId, ownerUserId, creationDateTm, score);
            posts.push_back(p);
        }
    }
}

// Some data for the map
struct UserData {
    std::string displayName;
    long long int totalScore;
};

class comparator
{
public:
    int operator() (const UserData& p1, const UserData& p2)
    {
        return p1.totalScore > p2.totalScore;
    }
};

// Returns pair of topUsersByQuestions, topUsersByAnswers
std::pair<std::vector<UserData>, std::vector<UserData> > getTopScoringUsers(
    const std::unordered_map<std::string, std::string> &users,
    const std::vector<Post> &posts) {

    // QUESTIONS
    std::map<std::string, UserData> questions;
    // ANSWERS
    std::map<std::string, UserData> answers;
    std::priority_queue <struct UserData, std::vector<struct UserData>, comparator> pq;
    // Calculate the total score of questions per user
    for (const auto &p : posts) {
        if (users.find(p.ownerUserId)!=users.end()) {
            questions[p.ownerUserId].displayName = users.find(p.ownerUserId)->second;
            answers[p.ownerUserId].displayName = users.find(p.ownerUserId)->second;
            if (p.postTypeId == Post::QUESTION) {
                questions[p.ownerUserId].totalScore += p.score;
            }else if(p.postTypeId == Post::ANSWER){
                answers[p.ownerUserId].totalScore += p.score;
            }
        }
    }

    //use heap store top 10 results
    for (const auto& it : questions) {
        if(pq.size()>=TOP_USERS){
            if(pq.top().totalScore < it.second.totalScore){
                pq.pop();   
                UserData data = {it.second.displayName,it.second.totalScore};
                pq.push(data);
            }
        }else{
            UserData data = {it.second.displayName,it.second.totalScore};
            pq.push(data);
        }
        
    }

    //put it back to vector
    std::vector<UserData> topUsersByQuestions(TOP_USERS);
    int j = TOP_USERS-1;
    while(!pq.empty()){
        topUsersByQuestions[j--] = pq.top();
        pq.pop();
    }

    pq = std::priority_queue<struct UserData, std::vector<struct UserData>, comparator>(); 

    //use heap store top 10 results
    for (const auto& it : answers) {
        if(pq.size()>=TOP_USERS){
            if(pq.top().totalScore < it.second.totalScore){
                pq.pop();   
                UserData data = {it.second.displayName,it.second.totalScore};
                pq.push(data);
            }
        }else{
            UserData data = {it.second.displayName,it.second.totalScore};
            pq.push(data);
        }
        
    }

    //put it back to vector
    std::vector<UserData> topUsersByAnswers(TOP_USERS);
    j = TOP_USERS-1;
    while(!pq.empty()){
        topUsersByAnswers[j--] = pq.top();
        pq.pop();
    }
    return std::make_pair(topUsersByQuestions, topUsersByAnswers);
}

int main(int argv, char** argc) {

    if (argv != 3) {
        std::cout << "ERROR: usage: " << argc[0] << "<users file>" << " <posts file>" << std::endl;
        exit(1);
    }

    // Keep track of all users
    std::unordered_map<std::string, std::string> users;

    // Keep track of all posts
    std::vector<Post> posts;

    // Parse and load posts and user data
    std::string usersFile = argc[1];
    std::string postsFile = argc[2];
     // Minimum valid date
    std::chrono::system_clock::time_point sixMonthsAgo = addMonthsToNow(-PAST_MONTHS);
    readUsers(usersFile, users);
    readPosts(postsFile, posts, sixMonthsAgo);
   
    auto topUsers = getTopScoringUsers(users, posts);

    std::vector<UserData> topUsersByQuestions = topUsers.first;
    std::vector<UserData> topUsersByAnswers = topUsers.second;

    std::cout << "Top " << TOP_USERS << " users by total score on questions asked from the past " << PAST_MONTHS << " months" << std::endl;
    for (const auto &q : topUsersByQuestions) {
        std::cout << q.totalScore << '\t' << q.displayName << std::endl;
    }
    std::cout << std::endl << std::endl;

    std::cout << "Top " << TOP_USERS << " users by total score on answers asked from the past " << PAST_MONTHS << " months" << std::endl;
    for (const auto &q : topUsersByAnswers) {
        std::cout << q.totalScore << '\t' << q.displayName << std::endl;
    }
    std::cout << std::endl << std::endl;

    return 0;
}
