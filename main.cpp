// Copyright Andrew Bernal 2023
#include <curl/curl.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class chessNode {
 public:
    chessNode(int wInp, int bInp, int dInp, std::string moveInp) {
        whiteWin = wInp;
        blackWin = bInp;
        drawn = dInp;
        UCImove = moveInp;
    }
    chessNode* getChild(int index) {
        return children[index];
    }
    void addChild(chessNode* toAdd) {
        children.push_back(toAdd);
    }
    std::string getMove() {
        return UCImove;
    }
    int getNumChildren() {
        return children.size();
    }
 private:
    std::string UCImove;
    int whiteWin;
    int blackWin;
    int drawn;
    std::vector<chessNode*> children;
};

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

void buildTree(chessNode* node, const std::string& FEN, const std::string& moves, bool whiteToMove, int totalGamesFromStartingPosition) {
    CURL *curl = curl_easy_init();
    if (!curl) { return; }

    std::string url = "https://explorer.lichess.ovh/lichess?variant=standard&speeds=blitz,rapid,classical&ratings=2200,2500&fen=";
    url += FEN;

    if (!moves.empty()) {
        url += "&play=";
        url += moves;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    std::string response_string;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    json j;

    bool shouldContinue = false;
    do {
        try {
            // rate limiting to follow the lichess api's request
            long response_code = 0;
            do {
                CURLcode res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(15));
                    return;
                }

                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                if (response_code == 429) {
                    std::cerr << "Rate limited. Waiting for 60 seconds before retrying..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(61));
                }
            } while (response_code == 429);

            // print for debugging
            // std::cout << response_string << "\n";

            try {
                j = json::parse(response_string);
                shouldContinue = false;
            } catch (const json::parse_error& e) {
                shouldContinue = true;
            }
        } catch (...) {
            std::cerr << "Something went wrong. Trying again in 15 seconds." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    } while (shouldContinue);
   

    // int totalGames = j["white"].get<int>() + j["black"].get<int>() + j["draws"].get<int>();

    if (whiteToMove) {
        // Select the highest win rate white move from the top 3 most played moves
        std::vector<json> topMoves(j["moves"].begin(), j["moves"].end());
        std::sort(topMoves.begin(), topMoves.end(), [](const json& a, const json& b) {
            int aTotal = a["white"].get<int>() + a["black"].get<int>() + a["draws"].get<int>();
            int bTotal = b["white"].get<int>() + b["black"].get<int>() + b["draws"].get<int>();
            return aTotal > bTotal;
        });
        topMoves.resize(std::min<size_t>(3, topMoves.size()));

        auto it = std::max_element(topMoves.begin(), topMoves.end(), [](const json& a, const json& b) {
            int aTotal = a["white"].get<int>() + a["black"].get<int>() + a["draws"].get<int>();
            int bTotal = b["white"].get<int>() + b["black"].get<int>() + b["draws"].get<int>();
            double aWinRate = aTotal == 0 ? 0 : static_cast<double>(a["white"].get<int>()) / aTotal;
            double bWinRate = bTotal == 0 ? 0 : static_cast<double>(b["white"].get<int>()) / bTotal;
            return aWinRate < bWinRate;
        });
        const json& move = *it;

        int whiteWins = move["white"];
        int blackWins = move["black"];
        int draws = move["draws"];

        chessNode* child = new chessNode(whiteWins, blackWins, draws, move["uci"]);
        node->addChild(child);

        std::string newMoves = moves;
        if (!newMoves.empty()) { newMoves += ","; }
        newMoves += move["uci"];

        buildTree(child, FEN, newMoves, !whiteToMove, totalGamesFromStartingPosition);
    } else {
        // Select all of the black moves with 1/1000 frequency of being played in the starting position
        for (const auto& move : j["moves"]) {
            int whiteWins = move["white"];
            int blackWins = move["black"];
            int draws = move["draws"];
            int totalChildGames = whiteWins + blackWins + draws;

            double probability = static_cast<double>(totalChildGames) / totalGamesFromStartingPosition;
            if (probability > 0.001) {
                chessNode* child = new chessNode(whiteWins, blackWins, draws, move["uci"]);
                node->addChild(child);

                std::string newMoves = moves;
                if (!newMoves.empty()) { newMoves += ","; }
                newMoves += move["uci"];

                buildTree(child, FEN, newMoves, !whiteToMove, totalGamesFromStartingPosition);
            }
        }
    }

    curl_easy_cleanup(curl);
}

int getStartingTotalNumGames(std::string FEN) {
    CURL *curl = curl_easy_init();
    if (!curl) { return 1; }

    std::string url = "https://explorer.lichess.ovh/lichess?variant=standard&speeds=blitz,rapid,classical&ratings=2200,2500&fen=";
    url += FEN;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    std::string response_string;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return 1;
    }
    json j = json::parse(response_string);
    int totalGamesFromStartingPosition = j["white"].get<int>() + j["black"].get<int>() + j["draws"].get<int>();
    return totalGamesFromStartingPosition;
}

std::string url_encode(const std::string &value) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (char c : value) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
    } else {
      escaped << std::uppercase;
      escaped << '%' << std::setw(2) << int((unsigned char) c);
      escaped << std::nouppercase;
    }
  }

  return escaped.str();
}

void traverseTree(chessNode* root, std::string pgn, std::ofstream& outputFile) {
    pgn += root->getMove();
    pgn += " ";
    if (root->getNumChildren() <= 0) {
        pgn += "\n";
        outputFile << pgn;
    }
    for(int i = 0; i < root->getNumChildren(); i++) {
        traverseTree(root->getChild(i), pgn, outputFile);
    }
}

int main() {
    //std::string FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR%20w%20KQkq%20-%200%201";
    std::string FEN = "rnbqkbnr/ppp2ppp/4p3/3p4/3PP3/2N5/PPP2PPP/R1BQKBNR b KQkq - 1 3";
    // std::cout << "Enter the FEN: ";
    // std::getline(std::cin, FEN);

    std::string htmlFEN = url_encode(FEN);
    bool whiteToMove = false;
    chessNode root(0, 0, 0, "");

    int totalGamesFromStart = getStartingTotalNumGames(htmlFEN);
    buildTree(&root, htmlFEN, "", whiteToMove, totalGamesFromStart);

    std::ofstream ofs("outputPGN.txt");
    traverseTree(&root, "", ofs);
    ofs.close();

    return 0;
}
